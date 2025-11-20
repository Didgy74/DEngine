use std::ffi::{OsStr, OsString};
use std::fmt::Debug;
use std::{env, fs};
use std::fs::File;
use std::io::{BufRead, BufReader, Error, ErrorKind};
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::sync::{Arc, Mutex};
use std::sync::atomic::{AtomicUsize, Ordering};
use std::time::{SystemTime};
use clap::Parser;
use walkdir::WalkDir;

use colored::*;

use rayon::prelude::*;
use rayon::ThreadPoolBuilder;

const CACHE_FILE_NAME: &str = ".last_build_info.txt";
const MAX_ERRORS: usize = 2;

// These includes are always relative to the input directory.
// TODO: This is not very good design.
const SLANG_ADDITIONAL_INCLUDES: [&str; 1] = [
    "lib"
];

/// Shader processing tool for DEngine
#[derive(Parser, Debug)]
#[command(author, version, about)]
struct Args {
    /// Input directory containing shaders
    #[arg(short, long)]
    input: PathBuf,

    /// Output directory for compiled shaders
    #[arg(short, long)]
    output: PathBuf,

    #[arg(short, long)]
    verbose: bool,

    /// Force build
    #[arg(short, long)]
    force: bool,

    /// Path to slangc executable
    #[arg(long)]
    slangc: Option<PathBuf>,

    /// Maximum number of threads to use. Must be greater than 0.
    #[arg(short, long)]
    threads: Option<usize>,
}
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let args = Args::parse();

    if args.verbose {
        println!("VERBOSE: Working directory: {}", env::current_dir()?.display());
    }

    validate_input_args(&args)?;

    if let Some(count) = args.threads {
        ThreadPoolBuilder::new()
            .num_threads(count) // <-- set max threads here
            .build_global()
            .expect("Failed to build global thread pool");
    }


    let latest_timestamp_input = compute_latest_modified_time(&args.input)?;
    let latest_secs = latest_timestamp_input
        .duration_since(SystemTime::UNIX_EPOCH)?
        .as_secs();
    if !args.force {
        let read_cache_result = read_cached_timestamp(&args.output);
        if read_cache_result.is_err() && args.verbose {
            println!(
                "Could not read existing cache timestamp. Forcing full rebuild. Reason: {}",
                read_cache_result.as_ref().err().unwrap());
        } else if let Ok(cached_timestamp) = &read_cache_result {
            let cached_secs = cached_timestamp
                .duration_since(SystemTime::UNIX_EPOCH)?
                .as_secs();
            if cached_secs >= latest_secs {
                println!("Assets are up to date. Skipping.");
                return Ok(());
            }
        }
    } else {
        if args.verbose {
            logging_function(
                "",
                &[String::from("Received force flag. Forcing full rebuild.")]);
        }
    }

    if args.output.exists() {
        fs::remove_dir_all(&args.output)?;
    }
    fs::create_dir_all(&args.output)?;

    // TODO: Is this correct?
    let actual_slangc = args.slangc.as_deref().unwrap_or(Path::new("slangc"));

    process_all_slang_files(
        &args.input,
        &args.output,
        &actual_slangc,
        args.verbose.then_some(&logging_function))?;

    copy_non_slang_files(
        &args.input,
        &args.output)?;

    write_cached_timestamp(
        &args.output,
        latest_timestamp_input)?;

    Ok(())
}

// Must be thread safe.
fn logging_function(msg: &str, verbose_logs: &[String]) {
    let mut all = String::new();
    if !msg.is_empty() {
        all.push_str(msg);
    }
    if !msg.is_empty() && !verbose_logs.is_empty() {
        all.push_str("\n");
    }
    if !verbose_logs.is_empty() {
        all.push_str(verbose_logs
            .iter()
            .map(|str| format!("VERBOSE: {}", str))
            .collect::<Vec<_>>()
            .join("\n")
            .as_str());
    }
    assert!(!all.is_empty());
    println!("{}", all);
}
fn validate_input_args(args: &Args) -> Result<(), Error> {
    if !args.input.exists() {
        return Err(Error::new(ErrorKind::NotFound, "Input directory does not exist"));
    }

    if !args.input.is_dir() {
        eprintln!("Input is not a directory");
        return Err(Error::new(ErrorKind::InvalidInput, "Input is not a directory"));
    }

    if args.input.join(CACHE_FILE_NAME).exists() {
        return Err(Error::new(
            ErrorKind::InvalidInput,
            "Input directory contains file '${CACHE_FILE_NAME}' which interferes with \
            caching mechanism"));
    }

    if args.input == args.output {
        return Err(Error::new(
            ErrorKind::InvalidInput,
            "Input and output directories are the same"));
    }

    // TODO: This does not take symlinks into account.
    if args.output.starts_with(&args.input) {
        return Err(Error::new(
            ErrorKind::InvalidInput,
            "Output directory is a subdirectory of input directory"));
    }

    let slangc_path = args.slangc.as_deref().unwrap_or(Path::new("slangc"));
    let output = Command::new(slangc_path)
        .arg("-v")
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
        .map_err(|e| Error::new(
            ErrorKind::NotFound,
            format!("Failed to run `{}`: {e}", slangc_path.display())),
        )?;
    if !output.status.success() {
        let stdout = String::from_utf8_lossy(&output.stdout);
        let stderr = String::from_utf8_lossy(&output.stderr);
        return Err(Error::new(
            ErrorKind::NotFound,
            format!(
                "`{}` did not run successfully ({})\n--- stdout ---\n{stdout}\n--- stderr ---\n{stderr}",
                slangc_path.display(),
                output.status)));
    }

    if let Some(threads) = args.threads {
        if threads < 1 {
            return Err(Error::new(
                ErrorKind::NotFound,
                "Thread count must be greater than 0"));
        }
    }

    Ok(())
}

fn compute_latest_modified_time(root: &Path) -> Result<SystemTime, Error> {
    let mut newest = SystemTime::UNIX_EPOCH;
    for entry in WalkDir::new(root)
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| e.file_type().is_file())
    {
        let metadata = entry.metadata()?;
        if let Ok(modified) = metadata.modified() {
            if modified > newest {
                newest = modified;
            }
        }
    }

    Ok(newest)
}

fn read_cached_timestamp(output_dir: &Path) -> Result<SystemTime, Error> {
    let cache_path = output_dir.join(CACHE_FILE_NAME);

    let contents = fs::read_to_string(cache_path)?;
    let seconds: u64 = contents.trim().parse()
        .map_err(|_| Error::new(ErrorKind::InvalidData, "Invalid cache file"))?;

    Ok(SystemTime::UNIX_EPOCH + std::time::Duration::from_secs(seconds))
}

fn write_cached_timestamp(output_dir: &Path, time: SystemTime) -> Result<(), Error> {
    let cache_path = output_dir.join(CACHE_FILE_NAME);

    let duration = time.duration_since(SystemTime::UNIX_EPOCH)
        .map_err(|_| Error::new(ErrorKind::Other, "SystemTime before epoch"))?;

    fs::write(cache_path, duration.as_secs().to_string())
}

fn process_all_slang_files(
    input_dir: &Path,
    output_dir: &Path,
    slangc: &Path,
    verbose_logging: Option<&(dyn Fn(&str, &[String]) + Send + Sync)>
) -> Result<(), Error> {
    let slang_files = find_slang_files(&input_dir);

    if let Some(log) = verbose_logging {
        log(
            "",
            &[format!("Found {} slang files", slang_files.len())]);
    }

    let progress = AtomicUsize::new(0);
    let error_count = AtomicUsize::new(0);
    let shared_failures = Arc::new(Mutex::new(Vec::<String>::new()));

    let additional_include_dirs = SLANG_ADDITIONAL_INCLUDES
        .iter()
        .map(|include| input_dir.join(include))
        .collect::<Vec<_>>();

    let _ = slang_files
        .par_iter()
        .try_for_each(|input_file| {
            // Stop early if too many errors
            if error_count.load(Ordering::Relaxed) >= MAX_ERRORS {
                return Err(());
            }

            let current = progress.fetch_add(1, Ordering::Relaxed) + 1;

            let relative = input_file.strip_prefix(input_dir).unwrap();

            let slang_process_result = process_slang_file(
                input_file,
                input_dir,
                output_dir,
                slangc,
                &additional_include_dirs);
            if let Some(log) = verbose_logging {
                let msg = format!("[{}/{}] {}", current, slang_files.len(), relative.display());
                log(
                    &msg,
                    &slang_process_result.verbose_logs)
            }
            match slang_process_result.result {
                Ok(_) => Ok(()),
                Err(e) => {
                    let count = error_count.fetch_add(1, Ordering::Relaxed) + 1;
                    let mut failures = shared_failures.lock().unwrap();
                    // If our failures already has reached max length,
                    // discard this one.
                    if failures.len() >= MAX_ERRORS {
                        return Err(());
                    }
                    failures.push(format!(
                        "{}:\n{}",
                        relative.display(),
                        e
                    ));

                    if count >= MAX_ERRORS {
                        Err(())
                    } else {
                        Ok(())
                    }
                }
            }
        });

    let failures = shared_failures.lock().unwrap();

    if !failures.is_empty() {
        eprintln!("{}", "\n=== Compilation Failed ===".red());
        for failure in failures.iter() {
            eprintln!("{}", failure.red());
            eprintln!("{}", "-------------------------".red());
        }

        if error_count.load(Ordering::Relaxed) >= MAX_ERRORS {
            eprintln!("Stopped early after {} errors.", MAX_ERRORS);
        }

        return Err(Error::new(
            ErrorKind::Other,
            "Shader compilation failed",
        ));
    }

    Ok(())
}

struct ProcessSlangFileResult {
    verbose_logs: Vec<String>,
    result: std::io::Result<()>,
}

fn process_slang_file(
    input_file: &Path,
    input_dir: &Path,
    output_dir: &Path,
    slangc: &Path,
    additional_include_dirs: &[PathBuf],
) -> ProcessSlangFileResult {
    let relative_input = input_file.strip_prefix(&input_dir).unwrap();
    let common_output_path = output_dir.join(&relative_input).with_extension("");
    let vtx_output_path = common_output_path.with_extension("vert.spv");
    let frag_output_path = common_output_path.with_extension("frag.spv");

    let mut return_value = ProcessSlangFileResult {
        verbose_logs: vec![],
        result: Ok(()),
    };

    // If the outpath directory does not exist, create it.
    // TODO: Could use better error-handling.
    fs::create_dir_all(common_output_path.parent().unwrap()).unwrap();

    let common_args = [
        "-target", "spirv",
        "-profile", "spirv_1_3", ]
        .map(OsString::from);

    let include_dir_args = additional_include_dirs
        .iter()
        .map(|dir| ["-I", dir.to_str().unwrap()])
        .flatten()
        .map(OsString::from)
        .collect::<Vec<_>>();

    let additional_vertex_args = [
        "-stage", "vertex",
        "-entry", "vsMain", ]
        .iter()
        .map(OsString::from)
        .collect::<Vec<_>>();

    let mut all_args: Vec<_> = common_args
        .iter()
        .chain(include_dir_args.iter())
        .chain(additional_vertex_args.iter())
        .cloned()
        .collect();
    all_args.push(OsString::from("-o"));
    all_args.push(vtx_output_path.into_os_string());
    all_args.push(input_file.as_os_str().to_os_string());
    let mut cmd = new_command_result(slangc.as_os_str(), &all_args);
    return_value.verbose_logs.push(
        format!("Running command: {}", cmd.command_string));
    let cmd_result = cmd.command.output();
    if let Err(e) = cmd_result {
        return_value.result = Err(e);
        return return_value;
    }
    let cmd_output = cmd_result.unwrap();
    if !cmd_output.status.success() {
        return_value.result = Err(Error::new(
            ErrorKind::InvalidInput,
            format!(
                "Error while compiling shader \n {}",
                String::from_utf8_lossy(&cmd_output.stderr))));
        return return_value;
    }

    let additional_frag_args = [
        "-stage", "fragment",
        "-entry", "fsMain",]
        .iter()
        .map(OsString::from)
        .collect::<Vec<_>>();
    let mut all_args = common_args
        .iter()
        .chain(include_dir_args.iter())
        .chain(additional_frag_args.iter())
        .cloned()
        .collect::<Vec<_>>();
    all_args.push(OsString::from("-o"));
    all_args.push(frag_output_path.into_os_string());
    all_args.push(input_file.as_os_str().to_os_string());
    let mut cmd = new_command_result(slangc.as_os_str(), &all_args);
    return_value.verbose_logs.push(
        format!("Running command: {}", cmd.command_string));
    let cmd_result = cmd.command.output();
    if let Err(e) = cmd_result {
        return_value.result = Err(e);
        return return_value;
    }
    let cmd_output = cmd_result.unwrap();
    if !cmd_output.status.success() {
        return_value.result = Err(Error::new(
            ErrorKind::InvalidInput,
            format!(
                "Error while compiling shader \n {}",
                String::from_utf8_lossy(&cmd_output.stderr))));
        return return_value;
    }

    return_value
}

fn find_slang_files<P: AsRef<Path>>(root: P) -> Vec<PathBuf> {
    WalkDir::new(root)
        .into_iter()
        .filter_map(|entry| entry.ok()) // Ignore errors
        .filter(|entry| entry.file_type().is_file())
        .filter(|entry| {
            entry.path()
                .extension()
                .and_then(|ext| ext.to_str())
                .map(|ext| ext.eq_ignore_ascii_case("slang"))
                .unwrap_or(false)
        })
        .filter(|entry| {
            // Exclude files where first line == "// dont precompile"
            if let Ok(file) = File::open(entry.path()) {
                let mut reader = BufReader::new(file);
                let mut first_line = String::new();
                if reader.read_line(&mut first_line).is_ok() {
                    return first_line.trim_end() != "// dont_precompile";
                }
            }
            // If we can't read it, keep it (or change to false if you prefer)
            true
        })
        .map(|entry| entry.into_path())
        .collect()
}

fn copy_non_slang_files(input_dir: &Path, output_dir: &Path) -> Result<(), Error> {
    for entry in WalkDir::new(input_dir)
        .into_iter()
        .filter_map(|e| e.ok())
    {
        let path = entry.path();
        let relative = path.strip_prefix(input_dir).unwrap();
        let target_path = output_dir.join(relative);

        if entry.file_type().is_dir() {
            fs::create_dir_all(&target_path)?;
            continue;
        }

        // Skip .slang files
        let is_slang = path.extension()
            .and_then(|ext| ext.to_str())
            .map(|ext| ext.eq_ignore_ascii_case("slang"))
            .unwrap_or(false);

        if is_slang {
            continue;
        }

        // Ensure parent dir exists
        if let Some(parent) = target_path.parent() {
            fs::create_dir_all(parent)?;
        }

        fs::copy(path, &target_path)?;
    }

    Ok(())
}

struct CommandResult {
    command: std::process::Command,
    command_string: String,
}

// Turn an executable path and arguments into a CommandResult
fn new_command_result<S: AsRef<OsStr>>(
    slangc: &OsStr,
    args: &[S],
) -> CommandResult {

    // Convert executable to string
    let exe_string = slangc.to_string_lossy();

    // Convert arguments to strings
    let args_string = args
        .iter()
        .map(|arg| arg.as_ref().to_string_lossy())
        .collect::<Vec<_>>()
        .join(" ");

    // Build full command string
    let command_string = if args_string.is_empty() {
        exe_string.to_string()
    } else {
        format!("{} {}", exe_string, args_string)
    };

    // Build actual command
    let mut cmd = Command::new(slangc);
    cmd.env_clear();
    cmd.args(args);

    CommandResult {
        command: cmd,
        command_string,
    }
}