param([bool]$overrideRuntimeLibraryType=$TRUE) #Must be the first statement in your script

$propPath = $(get-location).Path + "\grpc\vsprojects\global.props"

[xml]$prop = Get-Content $propPath

"Updating global.props"

# Disable warnings
$disableSpecificWarnings = $prop.Project.ItemDefinitionGroup.ClCompile.DisableSpecificWarnings

if($disableSpecificWarnings)
{
    "    DisableSpecificWarnings section already exists, skip add"
}
else
{
    "    Adding DisableSpecificWarnings section"
    $newElem = $prop.CreateElement("DisableSpecificWarnings", $prop.DocumentElement.NamespaceURI)
    $newElem.set_InnerXML("4267;4819;4291;%(DisableSpecificWarnings)")
    $prop.Project.ItemDefinitionGroup.ClCompile.AppendChild($newElem)
 }

# Set Windows platform 8.1
"    Adding WindowsTargetPlatformVersion section"
$propertyGroup = $prop.CreateElement("PropertyGroup", $prop.DocumentElement.NamespaceURI)
$propertyGroup.SetAttribute("Label", "Globals")
$prop.Project.AppendChild($propertyGroup)
$targetPlatform = $prop.CreateElement("WindowsTargetPlatformVersion", $prop.DocumentElement.NamespaceURI)
$targetPlatform.set_InnerXML("8.1")
$propertyGroup.AppendChild($targetPlatform)

$prop.Save($propPath)

# Removing static CRT settings, UE4 need CRT as DLL
if ($overrideRuntimeLibraryType)
{
    $files = Get-ChildItem ($(get-location).Path + "\grpc\vsprojects\vcxproj") *.vcxproj -recurse

    foreach ($file in $files)
    {
        [xml]$prop = Get-Content $file.FullName
        $isFound = $FALSE

        ($prop.Project.ChildNodes |Where-Object { "ItemDefinitionGroup" -eq $_.Name }) | ForEach-Object {
            $itemDefinitionGroup = $_
            ($itemDefinitionGroup.ChildNodes |Where-Object { "ClCompile" -eq $_.Name }) | ForEach-Object {
                $clCompile = $_
                ($clCompile.ChildNodes |Where-Object { "RuntimeLibrary" -eq $_.Name }) | ForEach-Object {
                    #"Remove" + $_.Name
                    [void]$_.ParentNode.RemoveChild($_)
                    $isFound = $TRUE
                }
            }
        }

        if ($isFound)
        {
            "Save " + $file.FullName
            $prop.Save($file.FullName)
        }
    }
}

# Removing static CRT settings, UE4 need CRT as DLL
if ($overrideRuntimeLibraryType)
{
    $files = Get-ChildItem ($(get-location).Path + "\grpc\third_party\protobuf\cmake\build\solution") *.vcxproj -recurse

    foreach ($file in $files)
    {
        [xml]$prop = Get-Content $file.FullName
        $isFound = $FALSE

        ($prop.Project.ChildNodes |Where-Object { "ItemDefinitionGroup" -eq $_.Name }) | ForEach-Object {
            $itemDefinitionGroup = $_
            ($itemDefinitionGroup.ChildNodes |Where-Object { "ClCompile" -eq $_.Name }) | ForEach-Object {
                $clCompile = $_
                ($clCompile.ChildNodes |Where-Object { "RuntimeLibrary" -eq $_.Name }) | ForEach-Object {
                    #"Remove" + $_.Name
                    [void]$_.ParentNode.RemoveChild($_)
                    $isFound = $TRUE
                }
            }
        }

        if ($isFound)
        {
            "Save " + $file.FullName
            $prop.Save($file.FullName)
        }
    }
}

# Currently unnecessary (we using "%devenv%" /upgrade instead) but may be helpful:

# Adding Platform Toolset VS 2017 for all project
#$files = Get-ChildItem ($(get-location).Path + "\grpc\vsprojects\vcxproj") *.vcxproj -recurse

#foreach ($file in $files)
#{
#    #"File " + $file.FullName
#    [xml]$prop = Get-Content $file.FullName
#
#    $vc15Props = $prop.CreateElement("PropertyGroup", $prop.DocumentElement.NamespaceURI)
#    $vc15Props.SetAttribute("Condition", "'`$(VisualStudioVersion)' == '15.0'")
#    $vc15Props.SetAttribute("Label", "Configuration")
#    $prop.Project.AppendChild($vc15Props)
#    $platformToolset = $prop.CreateElement("PlatformToolset", $prop.DocumentElement.NamespaceURI)
#    $platformToolset.set_InnerXML("v141")
#    $vc15Props.AppendChild($platformToolset)
#
#    $prop.Save($file.FullName)
#}
