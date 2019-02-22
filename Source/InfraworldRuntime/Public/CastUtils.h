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

#include <string>
#include <functional>
#include <chrono>

#include "GrpcIncludesBegin.h"

#include <google/protobuf/repeated_field.h>
#include <google/protobuf/map.h>
#include <grpc++/client_context.h>

#include "GrpcIncludesEnd.h"

// Should be imported to avoid long chrono-related instructions.
using std::chrono::milliseconds;
using std::chrono::system_clock;

//
// Generic casts. Should be specified in each header file for each generated structure.
//
namespace casts
{
    // ~~~~~ Unreal TMap and TArray, protobuf google::protobuf::RepeatedField and google::protobuf::Map) ~~~~~

    template<class OutT, class InT>
    using _UnrealMap = TMap<OutT, InT>;

    template<class OutT, class InT>
    using _ProtobufMap = google::protobuf::Map<OutT, InT>;

    template<class OutT>
    using _UnrealArray = TArray<OutT>;

    template<class OutT>
    using _ProtobufArray = google::protobuf::RepeatedField<OutT>;

    template<class OutT>
    using _ProtobufPtrArray = google::protobuf::RepeatedPtrField<OutT>;


    // ~~~~~ CAST PROTOTYPES (GENERIC) ~~~~~

    template<class OutT, class InT>
    FORCEINLINE OutT Proto_Cast(const InT &Item)
    {
        // WARNING!!!
        // If you're receiving an error message, telling you something like:
        //
        // error: no matching conversion for static_cast from 'xxx' to 'yyy'
        // return (OutT) t;
        //        ^~~~~
        // error: no matching function for call to 'Proto_Cast'
        // consider creating your own template specification of Proto_Cast<?>()
        // because by default it can only cast statically

        return static_cast<OutT>(Item);
    }

    // ~~~~~ CAST FUNCTIONS (MAPS) ~~~~~

    // TMap -> Protobuf Map
    template<class OutK, class OutV, class InK, class InV>
    FORCEINLINE _UnrealMap<OutK, OutV> Proto_MapCast(const _ProtobufMap<InK, InV>& Map)
    {
        _UnrealMap<OutK, OutV> OutMap;

        for (auto It = Map.cbegin(); It != Map.cend(); ++It)
            OutMap.Add(Proto_Cast<OutK>(It->first), Proto_Cast<OutV>(It->second));

        return OutMap;
    }

    // Protobuf Map -> TMap
    template<class OutK, class OutV, class InK, class InV>
    FORCEINLINE _ProtobufMap<OutK, OutV> Proto_MapCast(const _UnrealMap<InK, InV>& Map)
    {
        _ProtobufMap<OutK, OutV> OutMap;

        for (const TPair<InK, InV>& Pair : Map)
            OutMap.insert(google::protobuf::MapPair<OutK, OutV>(Proto_Cast<OutK>(Pair.Key), Proto_Cast<OutV>(Pair.Value)));

        return OutMap;
    }

    // ~~~~~ CAST FUNCTIONS (ARRAYS) ~~~~~

    template<class OutT, class InT>
    FORCEINLINE _ProtobufArray<OutT> Proto_ArrayCast(const _UnrealArray<InT>& Array)
    {
        // Allocate a protobuf array and reserve capacity
        _ProtobufArray<OutT> OutArray;
        OutArray.Reserve((int32)Array.Num());

        // Each item shall be individually casted to OutT
        for (const InT& Item : Array)
            OutArray.Add(Proto_Cast<OutT>(Item));

        return OutArray;
    }

    template<class OutT, class InT>
    FORCEINLINE _UnrealArray<OutT> Proto_ArrayCast(const _ProtobufArray<InT>& Array)
    {
        // Allocate a TArray<OutT> and reserve capacity
        _UnrealArray<OutT> OutArray;
        OutArray.Reserve((int32)Array.size());

        // Each item shall be individually casted to OutT
        std::for_each(Array.cbegin(), Array.cend(), [&OutArray](InT Item) {
            OutArray.Add(Proto_Cast<OutT>(Item));
        });

        return OutArray;
    }

    // Overload for _ProtobufPtrArray (google::protobuf::RepeatedPtrField<?>)
    // _ProtobufPtrArray (aka google::protobuf::RepeatedPtrField<OutT>)
    // is the same as RepeatedField<?>, but used for repeated strings or messages

    template<class OutT, class InT>
    FORCEINLINE _ProtobufPtrArray<OutT> Proto_PtrArrayCast(const _UnrealArray<InT>& Array)
    {
        // Allocate a protobuf array and reserve capacity
        _ProtobufPtrArray<OutT> OutArray;
        OutArray.Reserve((int32)Array.Num());

        // Each item shall be individually casted to OutT
        for (const InT& Item : Array)
        {
            const OutT& cast_result = Proto_Cast<OutT>(Item);
            
            // We need to create a new instance of OutT to be posessed by the OutArray.
            OutArray.AddAllocated(new OutT(cast_result));
        }

        return OutArray;
    }

    template<class OutT, class InT>
    FORCEINLINE _UnrealArray<OutT> Proto_PtrArrayCast(const _ProtobufPtrArray<InT>& Array)
    {
        // Allocate a TArray<OutT> and reserve capacity
        _UnrealArray<OutT> OutArray;
        OutArray.Reserve((int32)Array.size());

        // Each item shall be individually casted to OutT
        std::for_each(Array.cbegin(), Array.cend(), [&OutArray](InT Item) {
            OutArray.Add(Proto_Cast<OutT>(Item));
        });

        return OutArray;
    }

    // Casting enums value-wise does not require any specialization. Thou can override the template function.
    template<typename OutT, typename InT>
    FORCEINLINE OutT Proto_EnumCast(const InT &Item)
    {
        return static_cast<OutT>((int)Item);
    };

    // ~~~~~ CAST FUNCTIONS (BYTE ARRAY), in protobuf byte arrays are std::strings ~~~~~

    template <>
    FORCEINLINE FByteArray Proto_Cast(const std::string& String)
    {
        // Allocate a TArray<uint8>
        TArray<uint8> OutArray;
        
        // Put String's content into the array. We can not (and do not need) to cast away const qualifier.
        OutArray.Insert(reinterpret_cast<const uint8*>(String.c_str()), String.size(), 0);

        // Finally, wrap all data into FByteArray
        return FByteArray(OutArray);
    }

    template <>
    FORCEINLINE std::string Proto_Cast(const FByteArray& Item)
    {
        const TArray<uint8>& Arr = Item.Bytes;
        return std::string(reinterpret_cast<const char*>(Arr.GetData()), Arr.Num());
    }

    // ~~~~~ CAST FUNCTIONS (UNREAL STRING and PROTOBUF STRING) ~~~~~

    template <>
    FORCEINLINE std::string Proto_Cast(const FString& String)
    {
        return std::string(TCHAR_TO_UTF8(*String));
    }

    template <>
    FORCEINLINE FString Proto_Cast(const std::string& String)
    {
        return FString(String.c_str());
    }

    /**
     * Casts an UE4-compatible client context to the GRPC-compatible context.
     *
     * @note That grpc::ClientContext doesn't have a copy constructor, so grpc::ClientContext can not be returned from a
     *       ProtoCast<?>(). Thus the method should set an instance of grpc::ClientContext that already exist.
     * @param InContext Input UE4-compatible client context.
     * @param OutContext Output GRPC-compatible client context.
     */
    FORCEINLINE void CastClientContext(const FGrpcClientContext &InContext, grpc::ClientContext &OutContext)
    {
        // Cast and set metadata, checking for errors.
        for (const TPair<FString, FString>& Pair : InContext.Metadata)
        {
            if (Pair.Key.IsEmpty())
            {
                UE_LOG(LogTemp, Error, TEXT("Metadata key is empty for mapping '%s'->'%s' and thus won't be added to the client context. Behaviour is restricted by %s"),
                    *Pair.Key, *Pair.Value, TEXT("grpc/core/lib/surface/validate_metadata:80"));
            }
            else if (Pair.Key.StartsWith(":"))
            {
                UE_LOG(LogTemp, Error, TEXT("Metadata key statrs with ':' for mapping '%s'->'%s' and thus won't be added to the client context. Behaviour is restricted by %s"),
                    *Pair.Key, *Pair.Value, TEXT("grpc/core/lib/surface/validate_metadata:84"));
            }
            else
            {
                OutContext.AddMetadata(casts::Proto_Cast<std::string>(Pair.Key), casts::Proto_Cast<std::string>(Pair.Value));
            }

            // TODO: Add some other validation checks if necessary.
        }

        // Set deadline (Only if it has a positive value. It is -1 by default)
        if (InContext.DeadlineSeconds > .0f)
        {
            const int64 Milliseconds = static_cast<int64>(((double)InContext.DeadlineSeconds * 1000.0));
            OutContext.set_deadline(system_clock::now() + milliseconds(Milliseconds));
        }

        // Set boolean parameters
        OutContext.set_idempotent(InContext.bIdempotent);
        OutContext.set_cacheable(InContext.bCacheable);
        OutContext.set_wait_for_ready(InContext.bWaitForReady);

        // Set authority
        OutContext.set_authority(casts::Proto_Cast<std::string>(InContext.Authority));

        // Set Compression Algorithm
        OutContext.set_compression_algorithm(Proto_EnumCast<grpc_compression_algorithm>(InContext.GrpcCompressionAlgorithm));

        // Set Initial Metadata Corked
        OutContext.set_initial_metadata_corked(InContext.bInitialMetadataCorked);
    }

    FORCEINLINE void CastStatus(const grpc::Status& InStatus, FGrpcStatus& OutStatus)
    {
        OutStatus.ErrorCode = Proto_EnumCast<EGrpcStatusCode>(InStatus.error_code());
        OutStatus.ErrorMessage = Proto_Cast<FString>(InStatus.error_message());
        OutStatus.ErrorDetails = Proto_Cast<FString>(InStatus.error_details());
    }

    // Since we have no support for unsigned types in Blueprints, we need to

    template <>
    FORCEINLINE _UnrealArray<int32> Proto_ArrayCast(const _ProtobufArray<uint32>& Array)
    {
        // Allocate a TArray<OutT> and reserve capacity
        _UnrealArray<int32> OutArray;
        OutArray.Reserve((int32)Array.size());

        // Each item shall be individually casted to OutT
        std::for_each(Array.cbegin(), Array.cend(), [&OutArray](uint32 Item) {
            OutArray.Add(Proto_Cast<int32>(Item));
        });

        return OutArray;
    }

    template <>
    FORCEINLINE _ProtobufArray<uint32> Proto_ArrayCast(const _UnrealArray<int32>& Array)
    {
        // Allocate a protobuf array and reserve capacity
        _ProtobufArray<uint32> OutArray;
        OutArray.Reserve((int32)Array.Num());

        // Each item shall be individually casted to OutT
        for (const int32 Item : Array)
            OutArray.Add(Proto_Cast<uint32>(Item));

        return OutArray;
    }
}
