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
#include "InfraworldRuntime.h"

#include "RpcClient.h"

#include "RpcClientWorker.h"
#include "DefaultValueHelper.h"
#include "RunnableThread.h"

#include "Kismet/KismetStringLibrary.h"

// ===== Limited-accessible C++ only functions =====

bool CheckIp(const FString& Ip)
{
    const int32 NumOctets = 4;

    TArray<FString> ParsedOctets;
    ParsedOctets.Reserve(NumOctets);
    Ip.ParseIntoArray(ParsedOctets, TEXT("."));

    int32 parsedOctetdsNum = ParsedOctets.Num();

    if (parsedOctetdsNum == NumOctets)
    {
        const TRange<int32> OctetRange(0, 256);

        for (int32 i = 0; i < ParsedOctets.Num(); ++i)
        {
            const FString& Octet = ParsedOctets[i];
            int32 Out = -1;

            if (FDefaultValueHelper::ParseInt(Octet, Out))
            {
                if (!OctetRange.Contains(Out))
                {
                    UE_LOG(LogTemp, Error, TEXT("An %d'th octet (which is '%s') in the IPv4 address (which is '%s') is of range [0 - 256)"), i + 1, *Octet, *Ip);
                    return false;
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("'%s' in '%s' does not seems to be int32"), *Octet, *Ip);
                return false;
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Fatal, TEXT("Can not parse IPv4 address (which is '%s') into TArray<FString>, or invalid number of octets"), *Ip);
        return false;
    }

    return true;
}


// ============ RpcClient implementation ===========

bool URpcClient::Init(const FRpcClientInstantiationParameters& StartupParameters)
{
    if (bCanSendRequests)
    {
        UE_LOG(LogTemp, Error, TEXT("You're trying to initialize an RPC Client more than once"));
        return true;
    }

    // Note that the range [0, 65536) is upper-exclusive!
    const TRange<int32> ValidPortsRange(0, 65536);

    if (!CheckIp(StartupParameters.Ip))
    {
        UE_LOG(LogTemp, Error, TEXT("'%s' initialization failture: '%s' doesn't seem to be correct IPv4 address"),
               *(GetClass()->GetName()),
               *(StartupParameters.Ip)
               );
        return false;
    }

    if (!ValidPortsRange.Contains(StartupParameters.Port))
    {
        UE_LOG(LogTemp, Error, TEXT("'%s' initialization failture: '%d' does'nt seem to be correct port"),
               *(GetClass()->GetName()),
               StartupParameters.Port
               );
        return false;
    }

    // Do it if and only if the thread is not yet created.
    if (Thread == nullptr)
    {
        // Launch 'chaining' hierarchical init, which will init a superclass (a concrete implementation).
        HierarchicalInit();

        InnerWorker->DispatcherParameters = StartupParameters;

        // Retrieve and set an Error Message Queue
        if (InnerWorker)
        {
            InnerWorker->ErrorMessageQueue = &ErrorMessageQueue;

            const FString ThreadName(FString::Printf(TEXT("RPC Client Thread %s %d"), *(GetClass()->GetName()), FMath::RandRange(0, TNumericLimits<int32>::Max())));
            Thread = FRunnableThread::Create(InnerWorker.Get(), *ThreadName);

            bCanSendRequests = true;
            UE_LOG(LogTemp, Verbose, TEXT("Just made a thread: %s"), *ThreadName);
        }
        else
        {
            UE_LOG(LogTemp, Fatal, TEXT("An inner worker of %s wasn't initialized"), *(GetClass()->GetName()));
        }
    }

    return bCanSendRequests;
}

URpcClient::URpcClient() : InnerWorker(nullptr)
{
}

URpcClient::~URpcClient()
{
    UE_LOG(LogTemp, Verbose, TEXT("An instance of RPC Client has been destroyed. Still can send requests: %s"),
           *UKismetStringLibrary::Conv_BoolToString(CanSendRequests()));

    // Being called when GC'ed, should be called synchronously.
	if (CanSendRequests())
    {
		Stop(true);
    }
}

void URpcClient::Update()
{
    if (!ErrorMessageQueue.IsEmpty())
    {
        FRpcError RecievedError;
        ErrorMessageQueue.Dequeue(RecievedError);
        EventRpcError.Broadcast(this, RecievedError);

        // No need to call URpcClient::HierarchicalUpdate() if got any errors (Errors first)
    }
    else
    {
        HierarchicalUpdate();
    }
}

bool URpcClient::CanSendRequests() const
{
    return bCanSendRequests;
}

URpcClient* URpcClient::CreateRpcClient(TSubclassOf<URpcClient> Class, FRpcClientInstantiationParameters InstantiationParameters, UObject* Outer)
{
    UObject* const RealOuter = Outer ? Outer : (UObject*)GetTransientPackage();

    if (URpcClient* const CreatedClient = NewObject<URpcClient>(RealOuter, *Class))
    {
        bool IsClientInitialized = CreatedClient->Init(InstantiationParameters);
        if (!IsClientInitialized)
        {
            UE_LOG(LogTemp, Error, TEXT("Unable to initialize an RPC client (%s::Init() failed"), *(Class->GetName()));
            return nullptr;
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("An instance of %s has been created and initialized"), *(Class->GetName()));
            return CreatedClient;
        }
    }
    else
    {
        UE_LOG(LogTemp, Fatal, TEXT("Unable to create an instance of RPC client (NewObject<%s>() failed)"), *(Class->GetName()));
        return nullptr;
    }
}

void URpcClient::Stop(bool bSynchronous)
{
    if (Thread)
    {
        if (!InnerWorker->IsPendingStopped())
            InnerWorker->MarkPendingStopped();

        bCanSendRequests = false;
        UE_LOG(LogTemp, Verbose, TEXT("Scheduled to stop %s via setting 'bCanSendRequests = false'"), *(GetClass()->GetName()));

        // Should be synchronous in (almost) any case
        Thread->Kill(bSynchronous);

        delete Thread;
        Thread = nullptr;
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Can not call Stop() for an already stopped (or penfing asinchronously stopped) instance of '%s'"), *(GetClass()->GetName()));
    }
}
