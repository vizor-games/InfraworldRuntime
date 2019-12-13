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
#include "RpcClient.h"

#include "InfraworldRuntime.h"
#include "RpcClientWorker.h"
#include "GrpcUriValidator.h"

#include "Containers/Ticker.h"
#include "Misc/CoreDelegates.h"

#include "Misc/DefaultValueHelper.h"
#include "HAL/RunnableThread.h"
#include "Kismet/KismetStringLibrary.h"

// ============ RpcClient implementation ===========

bool URpcClient::Init(const FString& URI, UChannelCredentials* ChannelCredentials)
{
    if (bCanSendRequests)
    {
        UE_LOG(LogInfraworldRuntime, Error, TEXT("You're trying to initialize an RPC Client more than once"));
        return true;
    }
    
    FString ErrorMessage;
    if (!FGrpcUriValidator::Validate(URI, ErrorMessage))
    {
        UE_LOG(LogInfraworldRuntime, Error, TEXT("%s Unable to validate URI: %s"), *(GetClass()->GetName()), *ErrorMessage);
    }

    // Do it if and only if the thread is not yet created.
    if (Thread == nullptr)
    {
    	UE_LOG(LogInfraworldRuntime, Log, TEXT("RpcClient at [%p], Thread == nullptr, initializing"), this);

    	
        // Launch 'chaining' hierarchical init, which will init a superclass (a concrete implementation).
        HierarchicalInit();

    	UE_LOG(LogInfraworldRuntime, Log, TEXT("RpcClient at [%p], finished HierarchicalInit"), this);

    	
        // Retrieve and set an Error Message Queue
        if (InnerWorker)
        {
    		UE_LOG(LogInfraworldRuntime, Log, TEXT("RpcClient at [%p], InnerWorker = %p"), this, InnerWorker.Get());
        	
            InnerWorker->URI = URI;
            InnerWorker->ChannelCredentials = ChannelCredentials;

            InnerWorker->ErrorMessageQueue = &ErrorMessageQueue;

            const FString ThreadName(FString::Printf(TEXT("RPC Client Thread %s %d"), *(GetClass()->GetName()), FMath::RandRange(0, TNumericLimits<int32>::Max())));
            Thread = FRunnableThread::Create(InnerWorker.Get(), *ThreadName);

            bCanSendRequests = true;
            UE_LOG(LogInfraworldRuntime, Log, TEXT("Just made a thread: %s, address %p"), *ThreadName, InnerWorker.Get());
        }
        else
        {
            UE_LOG(LogInfraworldRuntime, Fatal, TEXT("An inner worker of %s wasn't initialized"), *(GetClass()->GetName()));
        }
    }

    if (CanSendRequests())
    {
        TickDelegateHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([this](float)
        {
            if (!ErrorMessageQueue.IsEmpty())
            {
                FRpcError ReceivedError;
                ErrorMessageQueue.Dequeue(ReceivedError);
                EventRpcError.Broadcast(this, ReceivedError);

                // No need to call URpcClient::HierarchicalUpdate() if got any errors (Errors first)
            }
            else
            {
                HierarchicalUpdate();
            }

            return true;
        }));
    }

    return bCanSendRequests;
}

URpcClient::URpcClient() : InnerWorker(nullptr), TickDelegateHandle()
{
}

URpcClient::~URpcClient()
{
    UE_LOG(LogInfraworldRuntime, Verbose, TEXT("An instance of RPC Client has been destroyed. Still can send requests: %s"),
           *UKismetStringLibrary::Conv_BoolToString(CanSendRequests()));
}

void URpcClient::Update()
{
    // Occasionally left blank
}

bool URpcClient::CanSendRequests() const
{
    return bCanSendRequests;
}

URpcClient* URpcClient::CreateRpcClient(TSubclassOf<URpcClient> Class, FRpcClientInstantiationParameters InstantiationParameters, UObject* Outer)
{
    const FString& URI = FString::Printf(TEXT("%s:%d"), *(InstantiationParameters.Ip), InstantiationParameters.Port);
    return CreateRpcClientUri(Class, URI, InstantiationParameters.ChannelCredentials, Outer);
}

URpcClient* URpcClient::CreateRpcClientUri(TSubclassOf<URpcClient> Class, const FString& URI, UChannelCredentials* ChannelCredentials, UObject* Outer)
{
    UObject* const RealOuter = Outer ? Outer : (UObject*)GetTransientPackage();
    
    if (URpcClient* const CreatedClient = NewObject<URpcClient>(RealOuter, *Class))
    {
		UE_LOG(LogInfraworldRuntime, Log, TEXT("Created RpcClient at [%p] with outer [%p]"), CreatedClient, RealOuter);
    	
        bool IsClientInitialized = CreatedClient->Init(URI, ChannelCredentials);
        if (!IsClientInitialized)
        {
            UE_LOG(LogInfraworldRuntime, Error, TEXT("Unable to initialize an RPC client (%s::Init() failed"), *(Class->GetName()));
            return nullptr;
        }
        else
        {
            UE_LOG(LogInfraworldRuntime, Verbose, TEXT("An instance of %s has been created and initialized"), *(Class->GetName()));
            return CreatedClient;
        }
    }
    else
    {
        UE_LOG(LogInfraworldRuntime, Fatal, TEXT("Unable to create an instance of RPC client (NewObject<%s>() failed)"), *(Class->GetName()));
        return nullptr;
    }
}

void URpcClient::BeginDestroy()
{
    // Being called when GC'ed, should be called synchronously.
    if (CanSendRequests())
    {
        Stop(true);
    }

    Super::BeginDestroy();
}

void URpcClient::Stop(bool bSynchronous)
{
    FRunnableThread* ThreadToStop = Thread.Exchange(nullptr);

    if (ThreadToStop)
    {
        if (!InnerWorker->IsPendingStopped())
            InnerWorker->MarkPendingStopped();

        bCanSendRequests = false;
        UE_LOG(LogInfraworldRuntime, Verbose, TEXT("Scheduled to stop %s via setting 'bCanSendRequests = false', address %p"), *(GetClass()->GetName()), InnerWorker.Get());

        // Should be synchronous in (almost) any case
        ThreadToStop->Kill(bSynchronous);

        delete ThreadToStop;
        ThreadToStop = nullptr;
        
        FTicker::GetCoreTicker().RemoveTicker(TickDelegateHandle);
    }
    else
    {
        UE_LOG(LogInfraworldRuntime, Error, TEXT("Can not call Stop() for an already stopped (or penfing asinchronously stopped) instance of '%s'"), *(GetClass()->GetName()));
    }
}
