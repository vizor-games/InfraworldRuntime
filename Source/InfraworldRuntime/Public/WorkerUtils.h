#pragma once

#include "GenUtils.h"
#include "CastUtils.h"
#include "Templates/Invoke.h"
#include "RpcClientWorker.h"

#include "GrpcIncludesBegin.h"

#include "ChannelProvider.h"
#include <grpc/support/log.h>
#include <grpc++/channel.h>
#include <grpc++/client_context.h>
#include <grpc++/completion_queue.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

#include "GrpcIncludesEnd.h"

template <class TStub>
class TStubbedRpcWorker : public RpcClientWorker
{
public:
	template <class TUnrealRequest, class TProtoRequest, class TUnrealResponse, class TProtoResponse, class TStubRequestFunctionPointer>
	TResponseWithStatus<TUnrealResponse> AsyncRequest(const TUnrealRequest Request, const FGrpcClientContext Context, const TStubRequestFunctionPointer MemberPointer)
	{
		const TProtoRequest ClientRequest = casts::Proto_Cast<TProtoRequest>(Request);
		
		grpc::ClientContext ClientContext;
		casts::CastClientContext(Context, ClientContext);

		grpc::CompletionQueue Queue;
	    grpc::Status Status;
		
	    std::unique_ptr<grpc::ClientAsyncResponseReader<TProtoResponse>> Rpc(Invoke(MemberPointer, Stub.get(), &ClientContext, ClientRequest, &Queue));

	    TProtoResponse Response;
	    Rpc->Finish(&Response, &Status, (void*)1);

	    void* got_tag;
	    bool ok = false;

	    while (true)
		{
			const std::chrono::seconds SingleWaitDuration = std::chrono::seconds(1);
			const std::chrono::time_point<system_clock> Deadline = std::chrono::system_clock::now() +
				SingleWaitDuration;
    		
	        const grpc::CompletionQueue::NextStatus NextStatus = Queue.AsyncNext(&got_tag, &ok, Deadline);

			if (NextStatus == grpc::CompletionQueue::NextStatus::GOT_EVENT)
			{
				break;
			}
    		
			if (IsPendingStopped())
			{
				ClientContext.TryCancel();
			}
		}
		
	    GPR_ASSERT(got_tag == (void*)1);
	    GPR_ASSERT(ok);

	    FGrpcStatus GrpcStatus;

	    casts::CastStatus(Status, GrpcStatus);
	    TResponseWithStatus<TUnrealResponse> Result(casts::Proto_Cast<TUnrealResponse>(Response), GrpcStatus);

	    return Result;
	}

protected:
	std::unique_ptr<TStub> Stub;
};
