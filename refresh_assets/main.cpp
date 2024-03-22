#include <iostream>
#include <filesystem>
#include <string>
#include <cstdlib>

namespace fs = std::filesystem;

// Check the OS and run the appropriate script
void runScript(const fs::path& dir) {
#ifdef _WIN32
	std::string cmd = "compile.cmd";
#else
	std::string cmd = "./compile.sh";
#endif
	auto scriptFilePath = dir;
	scriptFilePath.append(cmd);

	if (fs::exists(scriptFilePath) && fs::is_regular_file(scriptFilePath)) {
		std::string yo = std::string("cd ") + dir.string() + std::string(" && ") + cmd;
		// Make sure to flush std IO
		std::cout.flush();
		int result = system(yo.c_str());
		std::cout.flush();
		if (result != 0) {
			std::cout << "Failed to execute command." << std::endl;
			std::exit(result);
		}
	}
}

// Recursively check for the script in each directory
void checkAndRun(const fs::path& path) {
	for (const auto& entry : fs::recursive_directory_iterator(path)) {
		if (fs::is_directory(entry)) {
			runScript(entry.path());
		}
	}
}

int main(int argc, char* argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: " << argv[0] << " <path_to_directory>" << std::endl;
		return 1;
	}

	fs::path srcPath = argv[1];
	if (!fs::exists(srcPath) || !fs::is_directory(srcPath)) {
		std::cerr << "Provided path is not a valid directory." << std::endl;
		return 1;
	}

	checkAndRun(srcPath);

	fs::path destPath = fs::current_path() / srcPath.filename();

	if (fs::exists(destPath)) {
		fs::remove_all(destPath);  // Be careful, this will remove the directory and its contents
	}

	fs::copy(srcPath, destPath, fs::copy_options::recursive);

	std::cout << "Directory copied successfully!" << std::endl;

	return 0;
}