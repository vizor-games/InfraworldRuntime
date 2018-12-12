/*
 * Copyright 2018 Vizor Games LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

// #define TRACE

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net.NetworkInformation;
using System.Text;
using System.Text.RegularExpressions;
using UnrealBuildTool;

public class InfraworldRuntime : ModuleRules
{
    private UnrealTargetPlatform Platform;
    private UnrealTargetConfiguration Configuration;

    // name of root folders in the project folder
    private static readonly string GRPC_STRIPPED_FOLDER = "GrpcIncludes";
    private static readonly string GRPC_LIBS_FOLDER = "GrpcLibraries";

    private string INCLUDE_ROOT;
    private string LIB_ROOT;

    public class ModuleDepPaths
    {
        public readonly string[] HeaderPaths;
        public readonly string[] LibraryPaths;

        public ModuleDepPaths(string[] headerPaths, string[] libraryPaths)
        {
            HeaderPaths = headerPaths;
            LibraryPaths = libraryPaths;
        }

        public override string ToString()
        {
            return "Headers:\n" + string.Join("\n", HeaderPaths) + "\nLibs:\n" + string.Join("\n", LibraryPaths);
        }
    }

    [Conditional("DEBUG")]
    [Conditional("TRACE")]
    private void clog(params object[] objects)
    {
        Console.WriteLine(string.Join(", ", objects));
    }

    private IEnumerable<string> FindFilesInDirectory(string dir, string suffix = "")
    {
        List<string> matches = new List<string>();
        if (Directory.Exists(dir))
        {
            string[] files = Directory.GetFiles(dir);
            Regex regex = new Regex(".+\\." + suffix);

            foreach (string file in files)
            {
                if (regex.Match(file).Success)
                    matches.Add(file);
            }
        }

        return matches;
    }

    private string GetConfigurationString()
    {
        return (Configuration == UnrealTargetConfiguration.Shipping) ? "Release" : "Debug";
    }

    public ModuleDepPaths GatherDeps()
    {
        string RootPath = Path.GetFullPath(Path.Combine(ModuleDirectory, "../../"));

        INCLUDE_ROOT = Path.Combine(RootPath, GRPC_STRIPPED_FOLDER);
        LIB_ROOT = Path.Combine(RootPath, GRPC_LIBS_FOLDER);


        List<string> headers = new List<string>();
        List<string> libs = new List<string>();

        string PlatformLibRoot = "";


        if (Platform == UnrealTargetPlatform.Win64)
        {
            PlatformLibRoot = Path.Combine(LIB_ROOT, Platform.ToString());
            libs.AddRange(FindFilesInDirectory(PlatformLibRoot, "lib"));
        }
        else
        {
            PlatformLibRoot = Path.Combine(LIB_ROOT, Platform.ToString());
            libs.AddRange(FindFilesInDirectory(PlatformLibRoot, "a"));
        }

        clog("PlatformLibRoot: " + PlatformLibRoot);

        headers.Add(Path.Combine(INCLUDE_ROOT, "include"));
        headers.Add(Path.Combine(INCLUDE_ROOT, "third_party", "protobuf", "src"));

        return new ModuleDepPaths(headers.ToArray(), libs.ToArray());

    }

    public InfraworldRuntime(ReadOnlyTargetRules Target) : base(Target)
    {
        PublicDefinitions.Add("GOOGLE_PROTOBUF_NO_RTTI");
        PublicDefinitions.Add("GPR_FORBID_UNREACHABLE_CODE");
        PublicDefinitions.Add("GRPC_ALLOW_EXCEPTIONS=0");

        //TODO: We do this because in file generated_message_table_driven.h that located in protobuf sources 
        //TODO: line 174: static_assert(std::is_pod<AuxillaryParseTableField>::value, "");
        //TODO: causes С4647 level 3 warning __is_pod behavior change
        //TODO: UE4 threading some warnings as errors, and we have no chance to suppress this stuff
        //TODO: So, we don't want to change any third-party code, this why we add this definition
        PublicDefinitions.Add("__NVCC__");

        Platform = Target.Platform;
        Configuration = Target.Configuration;

        ModuleDepPaths moduleDepPaths = GatherDeps();
        Console.WriteLine(moduleDepPaths.ToString());

        PublicIncludePaths.AddRange(moduleDepPaths.HeaderPaths);
        PublicAdditionalLibraries.AddRange(moduleDepPaths.LibraryPaths);

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });

        AddEngineThirdPartyPrivateStaticDependencies(Target, "OpenSSL");
        AddEngineThirdPartyPrivateStaticDependencies(Target, "zlib");

        PrivateDependencyModuleNames.AddRange(new string[] { "CoreUObject", "Engine" });

    }
}
