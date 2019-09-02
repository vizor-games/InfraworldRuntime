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
#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"

#include <memory>

#include "ChannelCredentials.generated.h"

// namespace grpc
// {
//     class ChannelCredentials;
// }

USTRUCT(BlueprintType)
struct INFRAWORLDRUNTIME_API FRpcError
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadOnly, Category=Grpc)
    FString ErrorMessage;
};


/**
 * A channel credentials object encapsulates all the state needed by a client
 * to authenticate with a server for a given channel.
 * It can make various assertions, e.g., about the client’s identity, role
 * for all the calls on that channel.
 */
UCLASS(NotBlueprintable, NotBlueprintType, notplaceable, noteditinlinenew, hidedropdown, Transient, Abstract)
class INFRAWORLDRUNTIME_API UChannelCredentials : public UObject
{
    GENERATED_BODY()

public:
    /**
     * Builds credentials with reasonable defaults.
     *
     * \warning Only use these credentials when connecting to a Google endpoint.
     * Using these credentials to connect to any other service may result in this
     * service being able to impersonate your client for requests to Google
     * services.
     * @return Google Default Credentials
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vizor|RPC Credentials")
    static UChannelCredentials* MakeGoogleDefaultCredentials();

    /**
     * Builds SSL Credentials given SSL specific options.
     *
     * @param PemRootCerts
     *        The buffer containing the PEM encoding of the server root certificates. If
     *        this parameter is empty, the default roots will be used.  The default
     *        roots can be overridden using the \a GRPC_DEFAULT_SSL_ROOTS_FILE_PATH
     *        environment variable pointing to a file on the file system containing the
     *        roots.
     * @param PemPrivateKey
     *        The buffer containing the PEM encoding of the client's private key. This
     *        parameter can be empty if the client does not have a private key.
     * @param PemCertChain
     *        The buffer containing the PEM encoding of the client's certificate chain.
     *        This parameter can be empty if the client does not have a certificate
     *        chain.
     * @return Ssl Credentials
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vizor|RPC Credentials")
    static UChannelCredentials* MakeSslCredentials(
        UPARAM(DisplayName="PEM Root Certificates") FString PemRootCerts,
        UPARAM(DisplayName="PEM Private Key") FString PemPrivateKey,
        UPARAM(DisplayName="PEM Certificate Chain") FString PemCertChain
    );

    /**
     * Builds credentials for an unencrypted, unauthenticated channel.
     *
     * @return Insecure Channel Credentials
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vizor|RPC Credentials")
    static UChannelCredentials* MakeInsecureChannelCredentials();
};

/**
 * Builds credentials with reasonable defaults.
 *
 * \warning Only use these credentials when connecting to a Google endpoint.
 * Using these credentials to connect to any other service may result in this
 * service being able to impersonate your client for requests to Google
 * services.
 */
UCLASS()
class INFRAWORLDRUNTIME_API UGoogleDefaultCredentials : public UChannelCredentials
{
    GENERATED_BODY()

public:
};

/**
 * Builds SSL Credentials given SSL specific options
 */
UCLASS()
class INFRAWORLDRUNTIME_API USslCredentials : public UChannelCredentials
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Transient, DisplayName="PEM Root Certificates", Category=GrpcCerts)
    FString PemRootCerts;

    UPROPERTY(BlueprintReadOnly, Transient, DisplayName="PEM Private Key", Category=GrpcCerts)
    FString PemPrivateKey;

    UPROPERTY(BlueprintReadOnly, Transient, DisplayName="PEM Certificate Chain", Category=GrpcCerts)
    FString PemCertChain;
};

/**
 * Credentials for an unencrypted, unauthenticated channel
 */
UCLASS()
class INFRAWORLDRUNTIME_API UInsecureChannelCredentials : public UChannelCredentials
{
    GENERATED_BODY()

public:
};

/**
 * Instantiation parameters are used to create an RPC client.
 */
USTRUCT(BlueprintType)
struct INFRAWORLDRUNTIME_API FRpcClientInstantiationParameters
{
    GENERATED_BODY()

    /**
     * The IP address of the endpoint to connect to.
     */
    UPROPERTY(BlueprintReadWrite, Category=Endpoint)
    FString Ip;

    /**
     * The port of the endpoint to connect to.
     */
    UPROPERTY(BlueprintReadWrite, Category=EndpointPort)
    int32 Port;

    /**
     * Credentials to use for the created RPC client. If it does not hold
     * an object or is invalid, an error will be thrown.
     */
    UPROPERTY(BlueprintReadWrite, Category=Credentials)
    UChannelCredentials* ChannelCredentials;

    /**
     * Gets a GRPC URI for current ip address and port.
     *
     * @return URI for channel instantiation.
     */
    FString GetURI() const
    {
        return FString::Printf(TEXT("%s:%d"), *Ip, Port);
    }

    /**
     * Gets a string representation of current FRpcClientInstantiationParameters.
     *
     * @return FRpcClientInstantiationParameters string representation.
     */
    FString GetName() const
    {
        const FString& ParamsURI = GetURI();
        const FString& CredentialsClassName = ChannelCredentials ? *(ChannelCredentials->GetClass()->GetName()) : TEXT("nullptr (DANGER!)");

        return FString::Printf(TEXT("URI: %s, Credentials: %s"), *ParamsURI, *CredentialsClassName);
    }
};
