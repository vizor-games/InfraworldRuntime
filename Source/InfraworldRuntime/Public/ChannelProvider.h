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

#include <grpc++/channel.h>
#include <grpc++/create_channel.h>
#include <grpc++/security/credentials.h>

#include "RpcClientWorker.h"
#include "ChannelCredentials.h"


namespace channel
{
    FORCEINLINE bool WaitUntilChannelIsReady(const std::shared_ptr<grpc::Channel>& Channel, std::chrono::system_clock::time_point Deadline)
    {
        grpc_connectivity_state State = Channel->GetState(true);

        while (State != GRPC_CHANNEL_READY)
        {
            if (!Channel->WaitForStateChange(State, Deadline))
                return false;

            State = Channel->GetState(true);
        }

        return true;
    }

	FORCEINLINE bool WaitForConnection(float Seconds, const std::shared_ptr<grpc::Channel>& Channel)
    {
        bool IsConnected = false;

        const int64 Milliseconds = (int64)((double)Seconds * 1000.0);

        std::chrono::system_clock::time_point start_tp = std::chrono::system_clock::now();
        std::chrono::system_clock::time_point end_tp = std::chrono::system_clock::now() + std::chrono::milliseconds(Milliseconds);

        std::chrono::system_clock::time_point current_tp = start_tp;

        while (!IsConnected)
        {
            std::chrono::system_clock::time_point delta_tp = std::chrono::system_clock::now() + std::chrono::milliseconds(100);

            if (current_tp < end_tp)
                IsConnected = WaitUntilChannelIsReady(Channel, delta_tp);
            else
                break;

            current_tp = std::chrono::system_clock::now();
        }

        return IsConnected;
    }

	FORCEINLINE std::shared_ptr<grpc::ChannelCredentials> GetGrpcCredentials(UChannelCredentials* const Credentials)
    {
        // Check whether provided credentails are null.
        if (!Credentials)
        {
            UE_LOG(LogTemp, Error, TEXT("Provided credentials are NULL. (Did you forget to pass ChannelCredentials to instantiation parameters?). Replacement is grpc::InsecureChannelCredentials()."));
            return grpc::InsecureChannelCredentials();
        }

        // Classify the credentials
        if (Credentials->IsA<UGoogleDefaultCredentials>())
        {
            return grpc::GoogleDefaultCredentials();
        }
        else if (Credentials->IsA<UInsecureChannelCredentials>())
        {
            return grpc::InsecureChannelCredentials();
        }
        else if (const USslCredentials* const SslCredentials = Cast<USslCredentials>(Credentials))
        {
            grpc::SslCredentialsOptions Options;

            if (SslCredentials->PemRootCerts.Len() > 0)
                Options.pem_root_certs = TCHAR_TO_ANSI(*(SslCredentials->PemRootCerts));
            if (SslCredentials->PemPrivateKey.Len() > 0)
                Options.pem_private_key = TCHAR_TO_ANSI(*(SslCredentials->PemPrivateKey));
            if (SslCredentials->PemCertChain.Len() > 0)
                Options.pem_cert_chain = TCHAR_TO_ANSI(*(SslCredentials->PemCertChain));

            return grpc::SslCredentials(Options);
        }

        // Unknown credentials
        UE_LOG(LogTemp, Error, TEXT("Don't know how to process credentials:'%s'. Replacement is grpc::InsecureChannelCredentials()."), *(Credentials->GetClass()->GetName()));
        return grpc::InsecureChannelCredentials();
    }

	FORCEINLINE std::shared_ptr<grpc::Channel> CreateChannel(RpcClientWorker* Worker)
    {
        UChannelCredentials* const ChannelCredentials = Worker->ChannelCredentials;
        UE_CLOG(!ChannelCredentials, LogTemp, Fatal, TEXT("Channel Credentials mustn't be null"));

        const FString& URI = Worker->URI;
        UE_LOG(LogTemp, Display, TEXT("The following Channel Credentials is used: \"%s\". Connecting to: \"%s\""), *(ChannelCredentials->GetName()), *URI);

        std::shared_ptr<grpc::ChannelCredentials> GrpcCredentials = GetGrpcCredentials(ChannelCredentials);
        std::shared_ptr<grpc::Channel> Channel = grpc::CreateChannel(TCHAR_TO_ANSI(*URI), GrpcCredentials);

        bool bConnectionWasSuccessful = WaitForConnection(3, Channel);

        if (!bConnectionWasSuccessful)
        {
            Worker->DispatchError(TEXT("Connection failure"));
            return std::shared_ptr<grpc::Channel>(nullptr);
        }
        else
        {
            UE_LOG(LogTemp, Verbose, TEXT("Connection established!"));
        }

        return Channel;
    }
}
