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
#include "ChannelCredentials.h"

#include <grpc++/security/credentials.h>

// Since GRPC actively uses winapi, we need to forbid windows macros
// (such as GetMessage, MemoryBarrier, etc.) in our remaining code.
// To do it, we 'wrap' all the C++ file's including ANY GRPC header files
// content into Allow/Hide WindowsPlatformTypes.
// We're unable to 'isolate' the WinAPI usage within a single C++ file thanks to Unity Build.
#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"
#endif

std::shared_ptr<grpc::ChannelCredentials> UChannelCredentials::GetGrpcCredentials(UChannelCredentials* const Credentials)
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

        Options.pem_root_certs = TCHAR_TO_UTF8(*(SslCredentials->PemRootCerts));
        if (SslCredentials->PemPrivateKey.Len() > 0)
            Options.pem_private_key = TCHAR_TO_UTF8(*(SslCredentials->PemPrivateKey));
        if (SslCredentials->PemCertChain.Len() > 0)
            Options.pem_cert_chain = TCHAR_TO_UTF8(*(SslCredentials->PemCertChain));

        return grpc::SslCredentials(Options);
    }

    // Unknown credentials
    UE_LOG(LogTemp, Error, TEXT("Don't know how to process credentials:'%s'. Replacement is grpc::InsecureChannelCredentials()."), *(Credentials->GetClass()->GetName()));
    return grpc::InsecureChannelCredentials();
}

UChannelCredentials* UChannelCredentials::MakeGoogleDefaultCredentials()
{
    return NewObject<UGoogleDefaultCredentials>();
}

UChannelCredentials* UChannelCredentials::MakeSslCredentials(FString PemRootCerts, FString PemPrivateKey, FString PemCertChain)
{
    USslCredentials* const SslCredentials = NewObject<USslCredentials>();

    SslCredentials->PemRootCerts = PemRootCerts;
    SslCredentials->PemPrivateKey = PemPrivateKey;
    SslCredentials->PemCertChain = PemCertChain;

    return SslCredentials;
}

UChannelCredentials* UChannelCredentials::MakeInsecureChannelCredentials()
{
    return NewObject<UInsecureChannelCredentials>();
}

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif
