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
#include "GenUtils.generated.h"

// XX - major version
// YY - minor version
// ZZ - patch
#define INFRAWORLD_RUNTIME_VERSION 010000

/**
 * A wrapper for a TArray<uint8> used to avoid inner generics.
 */
USTRUCT(BlueprintType)
struct INFRAWORLDRUNTIME_API FByteArray
{
    GENERATED_USTRUCT_BODY()

    UPROPERTY(BlueprintReadWrite, Category=ArrayWrapper)
    TArray<uint8> Bytes;

    FByteArray()
    {
    }

    FByteArray(const TArray<uint8> &InBytes) :
        Bytes(InBytes)
    {
    }
};

// ~~~~~ Network CONTEXT ~~~~~

/**
 * The various compression algorithms supported by gRPC.
 */
UENUM(BlueprintType)
enum class EGrpcCompressionAlgorithm : uint8
{
    CompressNone,
    CompressDeflate,
    CompressGzip
};

/**
 * A ClientContext allows the person implementing a service client to:
 *
 * @todo Implement census/context if necessary.
 *
 * - Add custom metadata key-value pairs that will propagated to the server side.
 * - Control call settings such as compression and authentication.
 * - Initial and trailing metadata coming from the server.
 * - Get performance metrics (ie, census).
 *
 * Context settings are only relevant to the call they are invoked with, that
 * is to say, they aren't sticky. Some of these settings, such as the
 * compression options, can be made persistent at channel construction time
 * @see grpc::CreateCustomChannel()
 *
 * @warning ClientContext instances should not be reused across rpcs.
 * @see https://grpc.io/grpc/cpp/classgrpc_1_1_client_context.html#details
 */
USTRUCT(BlueprintType)
struct INFRAWORLDRUNTIME_API FGrpcClientContext
{
    GENERATED_USTRUCT_BODY()

    /**
     * Whether the optional Metadata attribute of the context is set.
     */
    UPROPERTY(EditAnywhere, meta=(InlineEditConditionToggle), Category=Metadata)
    bool bOverride_Metadata = false;

    /**
     * Note: Toggle an 'eye' if you DON'T NEED metadata at all.
     *
     * Add the (Key -> Value) pair to the metadata associated with a client call. These are made available at the server
     * side by the grpc::ServerContext::client_metadata() method.
     *
     * Restrictions:
     *  - Metadata keys can't be empty.
     *  - Metadata keys can't start with ':'.
     *
     * Note: This method should only be called before invoking the rpc.
     * Note: Keys are metadata keys. If meta_value is binary data, it must end in "-bin".
     * Note: Value The metadata value. If its value is binary, it must be base64-encoding
     *       https://tools.ietf.org/html/rfc4648#section-4 and Key must end in "-bin".
     */
    UPROPERTY(BlueprintReadWrite, meta=(editcondition=bOverride_Metadata), Category=Metadata)
    TMap<FString, FString> Metadata;

    /**
     * Set the deadline for the client call. Won't apply any deadline if values are (-INF, 0].
     * @warning This method should only be called before invoking the rpc.
     */
    UPROPERTY(BlueprintReadWrite, Category=Metadata)
    float DeadlineSeconds = -1.0f;

    /**
     * Set the per call authority header.
     * @see https://tools.ietf.org/html/rfc7540#section-8.1.2.3.
     */
    UPROPERTY(BlueprintReadWrite, Category=Metadata)
    FString Authority;

    /**
     * Set an algorithm to be the compression algorithm used for the client call.
     */
    UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category=Metadata)
    EGrpcCompressionAlgorithm GrpcCompressionAlgorithm;

    /**
     * EXPERIMENTAL: Set this request to be idempotent.
     */
    UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category=Metadata)
    bool bIdempotent;

    /**
     * EXPERIMENTAL: Set this request to be cacheable.
     */
    UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category=Metadata)
    bool bCacheable;

    /**
     * EXPERIMENTAL: Trigger wait-for-ready or not on this request.
     */
    UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category=Metadata)
    bool bWaitForReady;

    /**
     * Flag whether the initial metadata should be corked. If corked is true, then the initial metadata will be colasced
     * with the write of first message in the stream.
     */
    UPROPERTY(BlueprintReadWrite, AdvancedDisplay, Category=Metadata)
    bool bInitialMetadataCorked;
};


// ~~~~~ Wrappers for CONTEXT and STATUS ~~~~~

template<class TRequestType>
struct TRequestWithContext
{
    TRequestType Request;
    FGrpcClientContext Context;

    TRequestWithContext()
    {
    }

    TRequestWithContext(const TRequestType& InRequest, const FGrpcClientContext& InContext) :
        Request(InRequest),
        Context(InContext)
    {
    }
};

// Special constructor, automatically inferring arguments.
template<class T>
TRequestWithContext<T> TRequestWithContext$New(const T& InRequest, const FGrpcClientContext& InContext)
{
    return TRequestWithContext<T>(InRequest, InContext);
}

/**
 * GRPC error code.
 *
 * @see https://grpc.io/grpc/cpp/namespacegrpc.html#aff1730578c90160528f6a8d67ef5c43b
 */
UENUM(BlueprintType)
enum class EGrpcStatusCode : uint8
{
    /**
     * Not an error, returned on success.
     */
    Ok = 0,

    /**
     * The operation was cancelled (typically by the caller).
     */
    Cancelled = 1,

    /**
     * Unknown error. An example of where this error may be returned is if a
     * Status value received from another address space belongs to an error-space
     * that is not known in this address space. Also errors raised by APIs that
     * do not return enough error information may be converted to this error.
     */
    Unknown = 2,

    /**
     * Client specified an invalid argument. Note that this differs from
     * FailedPrecondition. InvalidArgument indicates arguments that are
     * problematic regardless of the state of the system (e.g., a malformed file
     * name).
     */
    InvalidArgument = 3,

    /**
     * Deadline expired before operation could complete. For operations that
     * change the state of the system, this error may be returned even if the
     * operation has completed successfully. For example, a successful response
     * from a server could have been delayed long enough for the deadline to
     * expire.
     */
    DeadlineExceeded = 4,

    /**
     * Some requested entity (e.g., file or directory) was not found.
     */
    NotFound = 5,

    /**
     * Some entity that we attempted to create (e.g., file or directory) already
     * exists.
     */
    AlreadyExists = 6,

    /**
     * The caller does not have permission to execute the specified operation.
     * PermissionDenied must not be used for rejections caused by exhausting
     * some resource (use ResourceExhausted instead for those errors).
     * PermissionDenied must not be used if the caller can not be identified
     * (use Unauthenticated instead for those errors).
     */
    PermissionDenied = 7,

    /**
     * The request does not have valid authentication credentials for the
     * operation.
     */
    Unauthenticated = 16,

    /**
     * Some resource has been exhausted, perhaps a per-user quota, or perhaps the
     * entire file system is out of space.
     */
    ResourceExhausted = 8,

    /**
     * Operation was rejected because the system is not in a state required for
     * the operation's execution. For example, directory to be deleted may be
     * non-empty, an rmdir operation is applied to a non-directory, etc.
     *
     * A litmus test that may help a service implementor in deciding
     * between FailedPrecondition, Aborted, and Unavailable:
     *  (a) Use Unavailable if the client can retry just the failing call.
     *  (b) Use Aborted if the client should retry at a higher-level
     *      (e.g., restarting a read-modify-write sequence).
     *  (c) Use FailedPrecondition if the client should not retry until
     *      the system state has been explicitly fixed. E.g., if an "rmdir"
     *      fails because the directory is non-empty, FailedPrecondition
     *      should be returned since the client should not retry unless
     *      they have first fixed up the directory by deleting files from it.
     *  (d) Use FailedPrecondition if the client performs conditional
     *      REST Get/Update/Delete on a resource and the resource on the
     *      server does not match the condition. E.g., conflicting
     *      read-modify-write on the same resource.
     */
    FailedPrecondition = 9,

    /**
     * The operation was aborted, typically due to a concurrency issue l *
     * sequencer check failures, transaction aborts, etc.
     *
     * See litmus test above for deciding between FailedPrecondition, Aborted,
     * and Unavailable.
     */
    Aborted = 10,

    /**
     * Operation was attempted past the valid range. E.g., seeking or reading
     * past end of file.
     *
     * Unlike InvalidArgument, this error indicates a problem that may be fixed
     * if the system state changes. For example, a 32-bit file system will
     * generate InvalidArgument if asked to read at an offset that is not in the
     * range [0,2^32-1], but it will generate OutOfRange if asked to read from
     * an offset past the current file size.
     *
     * There is a fair bit of overlap between FailedPrecondition and
     * OutOfRange. We recommend using OutOfRange (the more specific error)
     * when it applies so that callers who are iterating through a space can
     * easily look for an OutOfRange error to detect when they are done.
     */
    OutOfRange = 11,

    /**
     * Operation is not implemented or not supported/enabled in this service.
     */
    Unimplemented = 12,

    /**
     * Internal errors. Means some invariants expected by underlying System has
     * been broken. If you see one of these errors, Something is very broken.
     */
    Internal = 13,

    /**
     * The service is currently unavailable. This is a most likely a transient
     * condition and may be corrected by retrying with a backoff.
     */

    /**
     * See litmus test above for deciding between FailedPrecondition, Aborted,
     * and Unavailable.
     */
    Unavailable = 14,

    /**
     * Unrecoverable data loss or corruption.
     */
    DataLoss = 15
};

/**
 * Did it work? If it didn't, why?
 *
 * @see https://grpc.io/grpc/cpp/classgrpc_1_1_status.html#details
 */
USTRUCT(BlueprintType)
struct INFRAWORLDRUNTIME_API FGrpcStatus
{
    GENERATED_USTRUCT_BODY()

    /**
     * Return the instance's error code.
     */
    UPROPERTY(BlueprintReadOnly, Category=Status)
    EGrpcStatusCode ErrorCode;

    /**
     * Return the instance's error message.
     */
    UPROPERTY(BlueprintReadOnly, Category=Status)
    FString ErrorMessage;

    /**
     * The (binary) error details. Usually it contains a serialized google.rpc.Status proto.
     */
    UPROPERTY(BlueprintReadOnly, Category=Status)
    FString ErrorDetails;
};

template<class TResponseType>
struct TResponseWithStatus
{
    TResponseType Response;
    FGrpcStatus Status;

    TResponseWithStatus()
    {
    }

    TResponseWithStatus(const TResponseType& InResponse, const FGrpcStatus& InStatus) :
        Response(InResponse),
        Status(InStatus)
    {
    }
};
