///////////////////////////////////////////////////////////////////////////////
// USINGS
///////////////////////////////////////////////////////////////////////////////

using System;
using System.Collections.Generic;
using System.Linq;

///////////////////////////////////////////////////////////////////////////////
// ARGUMENTS
///////////////////////////////////////////////////////////////////////////////

var target = Argument("target", "Default");

///////////////////////////////////////////////////////////////////////////////
// SETTINGS
///////////////////////////////////////////////////////////////////////////////

var platforms = new [] { "Win32", "x64" }.ToList();
var configurations = new [] { "Debug", "Release" }.ToList();
var solution = "./EncWAVtoAC3.sln";
var scriptsDir = (DirectoryPath)Directory("./scripts");
var versionHeaderPath = (FilePath)File("./src/version.h");
var packageScriptPath = scriptsDir.CombineWithFilePath("Package");

///////////////////////////////////////////////////////////////////////////////
// VERSION
///////////////////////////////////////////////////////////////////////////////

var text = System.IO.File.ReadAllText(versionHeaderPath.FullPath);
var split = text.Split(new char [] { '\r', '\n' }, StringSplitOptions.RemoveEmptyEntries);
var major = split[0].Split(new char [] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
var minor = split[1].Split(new char [] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
var revision = split[2].Split(new char [] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
var build = split[3].Split(new char [] { ' ' }, StringSplitOptions.RemoveEmptyEntries)[2];
var version = major + "." + minor;

Information("Defined Version: {0}.{1}.{2}.{3}", major, minor, revision, build);
Information("Release Version: {0}", version);

///////////////////////////////////////////////////////////////////////////////
// TASKS
///////////////////////////////////////////////////////////////////////////////

Task("Clean")
    .Does(() =>
{
    CleanDirectory("./src/bin/");
    CleanDirectory("./src/obj/");
});

Task("Build")
    .IsDependentOn("Clean")
    .Does(() =>
{
    configurations.ForEach(configuration => {
        platforms.ForEach(platform => {
            Information("Building: {0}, {1} / {2}", solution, configuration, platform);
            MSBuild(solution, settings => {
                settings.SetConfiguration(configuration);
                settings.WithProperty("Platform", "\"" + platform + "\"");
                settings.SetVerbosity(Verbosity.Minimal);
            });
        });
    });
});

Task("Package")
    .IsDependentOn("Build")
    .Does(() =>
{
    StartProcess(packageScriptPath, new ProcessSettings { 
        Arguments = version, 
        WorkingDirectory = scriptsDir });
});

///////////////////////////////////////////////////////////////////////////////
// TARGETS
///////////////////////////////////////////////////////////////////////////////

Task("Default")
  .IsDependentOn("Package");

///////////////////////////////////////////////////////////////////////////////
// EXECUTE
///////////////////////////////////////////////////////////////////////////////

RunTarget(target);
