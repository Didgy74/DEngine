# Add project specific ProGuard rules here.
# By default, the flags in this file are appended to flags specified
# in ${ANDROID_SDK}/tools/proguard/proguard-android.txt
# You can edit the include path and order by changing the proguardFiles
# directive in build.gradle.
#
# For more details, see
#   http://developer.android.com/guide/developing/tools/proguard.html

# Add any project specific keep options here:

# If your project uses WebView with JS, uncomment the following
# and specify the fully qualified class name to the JavaScript interface
# class:
#-keepclassmembers class fqcn.of.javascript.interface.for.webview {
#   public *;
#}

-keepclasseswithmembernames,includedescriptorclasses class * {
    native <methods>;
}

-keep public class didgy.dengine.editor.DEngineActivity {
    public void nativeEvent_openSoftInput(java.lang.String, int);
    public void nativeEvent_hideSoftInput();
}