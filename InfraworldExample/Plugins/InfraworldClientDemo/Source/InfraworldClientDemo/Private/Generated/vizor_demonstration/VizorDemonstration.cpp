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
#include "InfraworldClientDemo.h"
#include "VizorDemonstration.h"
#include "RpcClientWorker.h"
#include "CastUtils.h"

#if PLATFORM_WINDOWS
    #include "AllowWindowsPlatformTypes.h"
#endif

#include <grpc/support/log.h>
#include <grpc++/channel.h>
#include <grpc++/create_channel.h>

#if PLATFORM_WINDOWS
    #pragma warning(push)
    #pragma warning (disable : 4125)// decimal digit terminates...
    #pragma warning (disable : 4647)// behavior change __is_pod...
    #pragma warning (disable : 4668)// 'symbol' is not defined as a preprocessor macro...
#endif

#include "Wrappers/vizor_demonstration/vizor_demonstration.pb.hpp"
#include "Wrappers/vizor_demonstration/vizor_demonstration.grpc.pb.hpp"

#include "ChannelProvider.h"

#if PLATFORM_WINDOWS
    #pragma warning(pop)
    #include "HideWindowsPlatformTypes.h"
#endif


// Enums:
// Structures:
namespace casts
{
    template <>
    FORCEINLINE FVizorDemonstration_HelloRequest Proto_Cast(const vizor_proto_demostration::HelloRequest& Item)
    {
        FVizorDemonstration_HelloRequest OutItem;

        // FVizorDemonstration_HelloRequest::Name <- HelloRequest::name
        // Struct (FString) <- Struct (std::string)
        OutItem.Name = Proto_Cast<FString>(Item.name());

        return OutItem;
    }

    template <>
    FORCEINLINE vizor_proto_demostration::HelloRequest Proto_Cast(const FVizorDemonstration_HelloRequest& Item)
    {
        vizor_proto_demostration::HelloRequest OutItem;

        // HelloRequest::name <- FVizorDemonstration_HelloRequest::Name
        // Struct (std::string) <- Struct (FString)
        OutItem.set_name(Proto_Cast<std::string>(Item.Name));

        return OutItem;
    }

    template <>
    FORCEINLINE FVizorDemonstration_HelloResponse Proto_Cast(const vizor_proto_demostration::HelloResponse& Item)
    {
        FVizorDemonstration_HelloResponse OutItem;

        // FVizorDemonstration_HelloResponse::Message <- HelloResponse::message
        // Struct (FString) <- Struct (std::string)
        OutItem.Message = Proto_Cast<FString>(Item.message());

        return OutItem;
    }

    template <>
    FORCEINLINE vizor_proto_demostration::HelloResponse Proto_Cast(const FVizorDemonstration_HelloResponse& Item)
    {
        vizor_proto_demostration::HelloResponse OutItem;

        // HelloResponse::message <- FVizorDemonstration_HelloResponse::Message
        // Struct (std::string) <- Struct (FString)
        OutItem.set_message(Proto_Cast<std::string>(Item.Message));

        return OutItem;
    }

    template <>
    FORCEINLINE FVizorDemonstration_ServerTimeRequest Proto_Cast(const vizor_proto_demostration::ServerTimeRequest& Item)
    {
        FVizorDemonstration_ServerTimeRequest OutItem;

        return OutItem;
    }

    template <>
    FORCEINLINE vizor_proto_demostration::ServerTimeRequest Proto_Cast(const FVizorDemonstration_ServerTimeRequest& Item)
    {
        vizor_proto_demostration::ServerTimeRequest OutItem;

        return OutItem;
    }

    template <>
    FORCEINLINE FVizorDemonstration_ServerTimeResponse Proto_Cast(const vizor_proto_demostration::ServerTimeResponse& Item)
    {
        FVizorDemonstration_ServerTimeResponse OutItem;

        // FVizorDemonstration_ServerTimeResponse::Hours <- ServerTimeResponse::hours
        // Primitive (int32) <- Primitive (google::protobuf::uint32)
        OutItem.Hours = Proto_Cast<int32>(Item.hours());

        // FVizorDemonstration_ServerTimeResponse::Minutes <- ServerTimeResponse::minutes
        // Primitive (int32) <- Primitive (google::protobuf::uint32)
        OutItem.Minutes = Proto_Cast<int32>(Item.minutes());

        // FVizorDemonstration_ServerTimeResponse::Seconds <- ServerTimeResponse::seconds
        // Primitive (int32) <- Primitive (google::protobuf::uint32)
        OutItem.Seconds = Proto_Cast<int32>(Item.seconds());

        // FVizorDemonstration_ServerTimeResponse::Timezone <- ServerTimeResponse::timezone
        // Struct (FString) <- Struct (std::string)
        OutItem.Timezone = Proto_Cast<FString>(Item.timezone());

        // FVizorDemonstration_ServerTimeResponse::Location <- ServerTimeResponse::location
        // Struct (FString) <- Struct (std::string)
        OutItem.Location = Proto_Cast<FString>(Item.location());

        return OutItem;
    }

    template <>
    FORCEINLINE vizor_proto_demostration::ServerTimeResponse Proto_Cast(const FVizorDemonstration_ServerTimeResponse& Item)
    {
        vizor_proto_demostration::ServerTimeResponse OutItem;

        // ServerTimeResponse::hours <- FVizorDemonstration_ServerTimeResponse::Hours
        // Primitive (google::protobuf::uint32) <- Primitive (int32)
        OutItem.set_hours(Proto_Cast<google::protobuf::uint32>(Item.Hours));

        // ServerTimeResponse::minutes <- FVizorDemonstration_ServerTimeResponse::Minutes
        // Primitive (google::protobuf::uint32) <- Primitive (int32)
        OutItem.set_minutes(Proto_Cast<google::protobuf::uint32>(Item.Minutes));

        // ServerTimeResponse::seconds <- FVizorDemonstration_ServerTimeResponse::Seconds
        // Primitive (google::protobuf::uint32) <- Primitive (int32)
        OutItem.set_seconds(Proto_Cast<google::protobuf::uint32>(Item.Seconds));

        // ServerTimeResponse::timezone <- FVizorDemonstration_ServerTimeResponse::Timezone
        // Struct (std::string) <- Struct (FString)
        OutItem.set_timezone(Proto_Cast<std::string>(Item.Timezone));

        // ServerTimeResponse::location <- FVizorDemonstration_ServerTimeResponse::Location
        // Struct (std::string) <- Struct (FString)
        OutItem.set_location(Proto_Cast<std::string>(Item.Location));

        return OutItem;
    }
}// end namespace 'casts'
class INFRAWORLDCLIENTDEMO_API HelloServiceRpcClientWorker : public RpcClientWorker
{

public:
    // Conduits and GRPC stub
    TConduit<TRequestWithContext<FVizorDemonstration_HelloRequest>, TResponseWithStatus<FVizorDemonstration_HelloResponse>>* HelloConduit;

    TConduit<TRequestWithContext<FVizorDemonstration_ServerTimeRequest>, TResponseWithStatus<FVizorDemonstration_ServerTimeResponse>>* ServerTimeConduit;

    std::unique_ptr<vizor_proto_demostration::HelloService::Stub> Stub;



    // Methods
    TResponseWithStatus<FVizorDemonstration_HelloResponse> Hello(const FVizorDemonstration_HelloRequest& Request, const FGrpcClientContext& Context)
    {
        vizor_proto_demostration::HelloRequest ClientRequest(casts::Proto_Cast<vizor_proto_demostration::HelloRequest>(Request));

        grpc::ClientContext ClientContext;
        casts::CastClientContext(Context, ClientContext);

        grpc::CompletionQueue Queue;
        grpc::Status Status;

        std::unique_ptr<grpc::ClientAsyncResponseReader<vizor_proto_demostration::HelloResponse>> Rpc(Stub->AsyncHello(&ClientContext, ClientRequest, &Queue));

        vizor_proto_demostration::HelloResponse Response;
        Rpc->Finish(&Response, &Status, (void*)1);

        void* got_tag;
        bool ok = false;

        GPR_ASSERT(Queue.Next(&got_tag, &ok));
        GPR_ASSERT(got_tag == (void*)1);
        GPR_ASSERT(ok);

        FGrpcStatus GrpcStatus;

        TResponseWithStatus<FVizorDemonstration_HelloResponse> Result(casts::Proto_Cast<FVizorDemonstration_HelloResponse>(Response), GrpcStatus);
        casts::CastStatus(Status, GrpcStatus);

        return Result;
    }

    TResponseWithStatus<FVizorDemonstration_ServerTimeResponse> ServerTime(const FVizorDemonstration_ServerTimeRequest& Request, const FGrpcClientContext& Context)
    {
        vizor_proto_demostration::ServerTimeRequest ClientRequest(casts::Proto_Cast<vizor_proto_demostration::ServerTimeRequest>(Request));

        grpc::ClientContext ClientContext;
        casts::CastClientContext(Context, ClientContext);

        grpc::CompletionQueue Queue;
        grpc::Status Status;

        std::unique_ptr<grpc::ClientAsyncResponseReader<vizor_proto_demostration::ServerTimeResponse>> Rpc(Stub->AsyncServerTime(&ClientContext, ClientRequest, &Queue));

        vizor_proto_demostration::ServerTimeResponse Response;
        Rpc->Finish(&Response, &Status, (void*)1);

        void* got_tag;
        bool ok = false;

        GPR_ASSERT(Queue.Next(&got_tag, &ok));
        GPR_ASSERT(got_tag == (void*)1);
        GPR_ASSERT(ok);

        FGrpcStatus GrpcStatus;

        TResponseWithStatus<FVizorDemonstration_ServerTimeResponse> Result(casts::Proto_Cast<FVizorDemonstration_ServerTimeResponse>(Response), GrpcStatus);
        casts::CastStatus(Status, GrpcStatus);

        return Result;
    }

    bool HierarchicalInit() override
    {
        std::shared_ptr<grpc::Channel> Channel = channel::CreateChannel(this);

        if (!Channel.get())
            return false;

        UE_LOG(LogTemp, Error, TEXT("%d"), (int32)Channel->GetState(false));

        // No need to call Super::HierarchicalInit(), it isn't required by design
        Stub = vizor_proto_demostration::HelloService::NewStub(Channel);

        HelloConduit->AcquireResponsesProducer();
        ServerTimeConduit->AcquireResponsesProducer();

        return true;
    }

    void HierarchicalUpdate() override
    {
        // No need to call Super::HierarchicalUpdate(), it isn't required by design
        if (!HelloConduit->IsEmpty())
        {
            TRequestWithContext<FVizorDemonstration_HelloRequest> WrappedRequest;
            HelloConduit->Dequeue(WrappedRequest);

            const TResponseWithStatus<FVizorDemonstration_HelloResponse>& WrappedResponse =
                Hello(WrappedRequest.Request, WrappedRequest.Context);
            HelloConduit->Enqueue(WrappedResponse);
        }

        if (!ServerTimeConduit->IsEmpty())
        {
            TRequestWithContext<FVizorDemonstration_ServerTimeRequest> WrappedRequest;
            ServerTimeConduit->Dequeue(WrappedRequest);

            const TResponseWithStatus<FVizorDemonstration_ServerTimeResponse>& WrappedResponse =
                ServerTime(WrappedRequest.Request, WrappedRequest.Context);
            ServerTimeConduit->Enqueue(WrappedResponse);
        }
    }
};


void UHelloServiceRpcClient::HierarchicalInit()
{
    // No need to call Super::HierarchicalInit(), it isn't required by design
    HelloServiceRpcClientWorker* const Worker = new HelloServiceRpcClientWorker();

    Worker->HelloConduit = &HelloConduit;
    HelloConduit.AcquireRequestsProducer();

    Worker->ServerTimeConduit = &ServerTimeConduit;
    ServerTimeConduit.AcquireRequestsProducer();

    InnerWorker = TUniquePtr<RpcClientWorker>(Worker);
}

void UHelloServiceRpcClient::HierarchicalUpdate()
{
    // No need to call Super::HierarchicalUpdate(), it isn't required by design
    if (!HelloConduit.IsEmpty())
    {
        TResponseWithStatus<FVizorDemonstration_HelloResponse> ResponseWithStatus;
        while (HelloConduit.Dequeue(ResponseWithStatus))
            EventHello.Broadcast(
                this,
                ResponseWithStatus.Response,
                ResponseWithStatus.Status
            );
    }

    if (!ServerTimeConduit.IsEmpty())
    {
        TResponseWithStatus<FVizorDemonstration_ServerTimeResponse> ResponseWithStatus;
        while (ServerTimeConduit.Dequeue(ResponseWithStatus))
            EventServerTime.Broadcast(
                this,
                ResponseWithStatus.Response,
                ResponseWithStatus.Status
            );
    }
}

bool UHelloServiceRpcClient::Hello(FVizorDemonstration_HelloRequest Request, const FGrpcClientContext& Context)
{
    if (!CanSendRequests())
        return false;

    HelloConduit.Enqueue(TRequestWithContext$New(Request, Context));
    return true;
}

bool UHelloServiceRpcClient::ServerTime(FVizorDemonstration_ServerTimeRequest Request, const FGrpcClientContext& Context)
{
    if (!CanSendRequests())
        return false;

    ServerTimeConduit.Enqueue(TRequestWithContext$New(Request, Context));
    return true;
}
