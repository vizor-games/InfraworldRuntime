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

#include "GrpcUriValidator.h"
#include "InfraworldRuntime.h"
#include "Misc/DefaultValueHelper.h"

class FGrpcUriValidator_Internal
{
public:
    static bool ValidatePort(const FString& MaybePort, FString& OutError);
    static bool ValidateIp(const FString& MaybeIpAddress, FString& OutError);
    static bool ValidateDomainName(const FString& MaybeDomainName, FString& OutError);
    
    static bool DoesHostLookLikeIp(const FString& MaybeIpAddress);
    
private:
    static bool TCharIsLetter(TCHAR character);
    static bool TCharIsDigit(TCHAR character);
};


/// FGrpcUriValidator_Internal interface

bool FGrpcUriValidator_Internal::ValidatePort(const FString& MaybePort, FString& OutError)
{
    int32 PortAsInteger;
    if (FDefaultValueHelper::ParseInt(MaybePort, PortAsInteger))
    {
        const TRange<int32> ValidPortRange(0, 0xFFFF);
        
        if (!ValidPortRange.Contains(PortAsInteger))
        {
            OutError = FString::Printf(TEXT("Invalid port number: \"%d\", must be within [%d - %d)"), PortAsInteger, 0, 0xFFFF);
            return false;
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Can not parse port \"%s\" into an integer. The format is invalid."), *MaybePort);
        return false;
    }
    
    return true;
}

bool FGrpcUriValidator_Internal::ValidateIp(const FString& MaybeIpAddress, FString& OutError)
{
    TArray<FString> Octets;
    MaybeIpAddress.ParseIntoArray(Octets, TEXT("."));
    
    if (Octets.Num() == 4)
    {
        const TRange<int32> OctetRange(0, 0xFF);
        for (const FString& Octet : Octets)
        {
            int32 Out;
            if (FDefaultValueHelper::ParseInt(Octet, Out))
            {
                if (!OctetRange.Contains(Out))
                {
                    OutError = FString::Printf(TEXT("An octet \"%s\" in the IPv4 address (which is \"%s\") is of range [0 - 256)"), *Octet, *MaybeIpAddress);
                    return false;
                }
            }
            else
            {
                OutError = FString::Printf(TEXT("\"%s\" in \"%s\" does not seems to be int32"), *Octet, *MaybeIpAddress);
                return false;
            }
        }
    }
    else
    {
        OutError = FString::Printf(TEXT("Can not parse IPv4 address (which is \"%s\") into TArray<FString>, or invalid number of octets"), *MaybeIpAddress);
        return false;
    }
    
    return true;
}

bool FGrpcUriValidator_Internal::ValidateDomainName(const FString& MaybeDomainName, FString& OutError)
{
    for (TCHAR Character : MaybeDomainName)
    {
        if (!TCharIsLetter(Character) && !TCharIsDigit(Character) && (Character != TEXT('-')) && (Character != TEXT('.')))
        {
            OutError = FString::Printf(TEXT("\"%s\" domain name contains forbidden character: \"%c\""), *MaybeDomainName, Character);
            return false;
        }
    }
    
    return true;
}

bool FGrpcUriValidator_Internal::DoesHostLookLikeIp(const FString& MaybeIpAddress)
{
    for (TCHAR Character : MaybeIpAddress)
    {
        if (!TCharIsDigit(Character) && (Character != TEXT('.')))
            return false;
    }
    
    return true;
}

bool FGrpcUriValidator_Internal::TCharIsLetter(TCHAR character)
{
    const bool bUpperCase = (character >= TEXT('A')) && (character <= TEXT('Z'));
    const bool bLowerCase = (character >= TEXT('a')) && (character <= TEXT('z'));
    
    return bUpperCase || bLowerCase;
}

bool FGrpcUriValidator_Internal::TCharIsDigit(TCHAR character)
{
    return (character >= TEXT('0')) && (character <= TEXT('9'));
}


/// FGrpcUriValidator interface

bool FGrpcUriValidator::Validate(const FString& MaybeGrpcUri, FString& OutError)
{
    static const FString SchemeSeparator(TEXT("://"));
    
    const int32 IndexOfSchemeSeparator = MaybeGrpcUri.Find(SchemeSeparator);
    if (IndexOfSchemeSeparator >= 0)
    {
        const FString& Scheme = MaybeGrpcUri.Mid(0, IndexOfSchemeSeparator);
        
        OutError = FString::Printf(TEXT("GRPC URI \"%s\" must not contain a URL scheme (\"%s\" provided). GRPC forbids explicit schemes."), *MaybeGrpcUri, *Scheme);
        return false;
    }
    
    const int32 PathSeparatorIndex = MaybeGrpcUri.Find(TEXT("/"));
    const bool bHasPathSeparator = PathSeparatorIndex >= 0;
    
    const int32 PortSeparatorIndex = MaybeGrpcUri.Find(TEXT(":"), ESearchCase::IgnoreCase, ESearchDir::FromEnd, (PathSeparatorIndex >= 0) ? PathSeparatorIndex : INDEX_NONE);
    const bool bHasPortSeparator = PortSeparatorIndex >= 0;
    
    FString GrpcHostName = TEXT("");
    FString GrpcPort = TEXT("80");
    
    if (bHasPortSeparator)
    {
        GrpcHostName = MaybeGrpcUri.Mid(0, PortSeparatorIndex);
        
        const int32 PortSubstringStart = PortSeparatorIndex + 1;
        if (bHasPathSeparator)
        {
            GrpcPort = MaybeGrpcUri.Mid(PortSubstringStart, (PathSeparatorIndex - PortSubstringStart));
        }
        else
        {
            GrpcPort = MaybeGrpcUri.Mid(PortSubstringStart);
        }
    }
    else
    {
        if (bHasPathSeparator)
        {
            GrpcHostName = MaybeGrpcUri.Mid(0, PathSeparatorIndex);
        }
        else
        {
            GrpcHostName = MaybeGrpcUri;
        }
    }
    
    if (bHasPathSeparator)
    {
        const FString& RestOfAddress = MaybeGrpcUri.Mid(PathSeparatorIndex);
        if (!RestOfAddress.IsEmpty())
        {
            OutError = FString::Printf(TEXT("Path of the \"%s\" uri, must be empty. Actually it is: \"%s\""), *MaybeGrpcUri, *RestOfAddress);
            return false;
        }
    }
    
    if (FGrpcUriValidator_Internal::DoesHostLookLikeIp(GrpcHostName))
    {
        // Validate as IP address
        if (!FGrpcUriValidator_Internal::ValidateIp(GrpcHostName, OutError))
            return false;
    }
    else
    {
        // Validate as domain name
        if (!FGrpcUriValidator_Internal::ValidateDomainName(GrpcHostName, OutError))
            return false;
    }
    
    // Anyway, validate port
    if (!FGrpcUriValidator_Internal::ValidatePort(GrpcPort, OutError))
        return false;
    
    return true;
}
