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
#include "Conduit.h"
#include "GenUtils.h"
#include "RpcClient.h"
#include "VizorDemonstration.generated.h"

USTRUCT(BlueprintType, meta=(DisplayName="VizorDemonstration HelloRequest"))
struct INFRAWORLDCLIENTDEMO_API FVizorDemonstration_HelloRequest
{
    GENERATED_USTRUCT_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY(BlueprintReadWrite, Transient)
    FString Name;
};

USTRUCT(BlueprintType, meta=(DisplayName="VizorDemonstration HelloResponse"))
struct INFRAWORLDCLIENTDEMO_API FVizorDemonstration_HelloResponse
{
    GENERATED_USTRUCT_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY(BlueprintReadWrite, Transient)
    FString Message;
};

USTRUCT(BlueprintType, meta=(DisplayName="VizorDemonstration ServerTimeRequest"))
struct INFRAWORLDCLIENTDEMO_API FVizorDemonstration_ServerTimeRequest
{
    GENERATED_USTRUCT_BODY()
};

USTRUCT(BlueprintType, meta=(DisplayName="VizorDemonstration ServerTimeResponse"))
struct INFRAWORLDCLIENTDEMO_API FVizorDemonstration_ServerTimeResponse
{
    GENERATED_USTRUCT_BODY()

public:
    // Conduits and GRPC stub
    UPROPERTY(BlueprintReadWrite, Transient)
    int32 Hours;

    UPROPERTY(BlueprintReadWrite, Transient)
    int32 Minutes;

    UPROPERTY(BlueprintReadWrite, Transient)
    int32 Seconds;

    UPROPERTY(BlueprintReadWrite, Transient)
    FString Timezone;

    UPROPERTY(BlueprintReadWrite, Transient)
    FString Location;
};

// Forward class definitions (for delegates)
class UHelloServiceRpcClient;

// Dispatcher delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEventHelloHelloService, UHelloServiceRpcClient*, RpcClient, const FVizorDemonstration_HelloResponse&, Response, FGrpcStatus, Status);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEventServerTimeHelloService, UHelloServiceRpcClient*, RpcClient, const FVizorDemonstration_ServerTimeResponse&, Response, FGrpcStatus, Status);


UCLASS()
class INFRAWORLDCLIENTDEMO_API UHelloServiceRpcClient : public URpcClient
{
    GENERATED_BODY()

public:
    // Conduits and GRPC stub
    TConduit<TRequestWithContext<FVizorDemonstration_HelloRequest>, TResponseWithStatus<FVizorDemonstration_HelloResponse>> HelloConduit;

    TConduit<TRequestWithContext<FVizorDemonstration_ServerTimeRequest>, TResponseWithStatus<FVizorDemonstration_ServerTimeResponse>> ServerTimeConduit;

    UPROPERTY(BlueprintAssignable, Category="Vizor|RPC Responses|HelloService")
    FEventHelloHelloService EventHello;

    UPROPERTY(BlueprintAssignable, Category="Vizor|RPC Responses|HelloService")
    FEventServerTimeHelloService EventServerTime;


    // Methods
    void HierarchicalInit() override;

    void HierarchicalUpdate() override;

    UFUNCTION(BlueprintCallable, Category="Vizor|RPC Requests|HelloService")
    bool Hello(FVizorDemonstration_HelloRequest Request, const FGrpcClientContext& Context);

    UFUNCTION(BlueprintCallable, Category="Vizor|RPC Requests|HelloService")
    bool ServerTime(FVizorDemonstration_ServerTimeRequest Request, const FGrpcClientContext& Context);
};
