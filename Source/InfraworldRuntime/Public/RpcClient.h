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


#include "Containers/Queue.h"
#include "Templates/SubclassOf.h"
#include "Templates/Atomic.h"

#include "RpcClientWorker.h"
#include "ChannelCredentials.h"

#include "RpcClient.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FRpcErrorSignature, URpcClient*, Dispatcher, const FRpcError&, Error);

/**
 * An RPC client used to interact with GRPC services from Blueprints and UE-compatible C++ code.
 * Being used as a base class for generated RPC clients, an RPC Client contains a set of methods to manage an RPC Channel.
 *
 * @note You should never instantiate it directly.
 *
 * You should use a Proto2Cpp converter to create GRPC wrappers for an every single service.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class INFRAWORLDRUNTIME_API URpcClient : public UObject
{
	GENERATED_BODY()

    bool Init(const FString& URI, UChannelCredentials* ChannelCredentials);

public:
	URpcClient();
	virtual ~URpcClient();

    // Being called in implementations
    virtual void HierarchicalInit() PURE_VIRTUAL(URpcClient::HierarchicalInit,);

    // Being called in implementations
    virtual void HierarchicalUpdate() PURE_VIRTUAL(URpcClient::HierarchicalUpdate,);

    /**
     * Stops and disables this instance of RPC Client.
     * This operation is irreversible, you can't either send or receive requests and responses after doing so.
     *
     * @param bSynchronous
     *        Stop and wait a sec. Oh when you look at me like that my darling - What did you expect?
     *        If seriously - you should think thousand times before unchecking this option, because
     *        you can experience heavy and unpredictable racing hazard.
     */
    UFUNCTION(BlueprintCallable, Category="Vizor|RPC Client")
    void Stop(bool bSynchronous = true);

    /**
     * An update function, used to tell a Client, when to check queries and dispatch messages.
     * In widget (for example) you should call it every update (or redraw/invalidate).
     *
     * @note that frequency, you're calling this method won't ever affect the speed of message processing.
     * @deprecated No need to call this function anymore.
     */
    UFUNCTION(BlueprintCallable, Category = "Vizor|RPC Client", meta = (DeprecatedFunction, DeprecationMessage = "No need to call this function anymore: Updates now are being dispatched automatically"))
    void Update();

    /**
     * Checks whether the RPC Client could send requests.
     *
     * @return True if the RPC Client is properly initialized and can send requests. If not - all requests will be ignored.
     */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category="Vizor|RPC Client", meta=(DisplayName="Can Send Requests?"))
    bool CanSendRequests() const;

    /**
     * Instantiates a new RPC Dispatcher. You should use this function, not 'Construct Object from Class', to properly initialize the instance.
     *
     * @param Class
     *        A subclass of RPC Client, that will be instantiated. Please don't select an abstract 'Rpc Client'.
     * @param InstantiationParameters
     *        Parameters, will be used for instantiation,
     * @param Outer
     *        An outer object, None (or empty) is valid as well, if so, GetTransientPackage() will be used to retrieve a static outer.
     * @return A newly created instance of RPC Client.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Vizor|RPC Client", meta=(DisplayName="Create RPC Client", DeterminesOutputType="Class", DeprecatedFunction, DeprecationMessage="Use function, accepting URI and ChannelCredentials instead"))
    static URpcClient* CreateRpcClient(TSubclassOf<URpcClient> Class, FRpcClientInstantiationParameters InstantiationParameters, UObject* Outer = nullptr);
    
    /**
     * Instantiates a new RPC Dispatcher. You should use this function, not 'Construct Object from Class', to properly initialize the instance.
     *
     * @param Class
     *        A subclass of RPC Client, that will be instantiated. Please don't select an abstract 'Rpc Client'.
     * @param URI
     *        Endpoint URI, will be used to establish connection.
     *        Must not start with 'http://' or 'https://' or any URL scheme.
     *        Must not have URL path, such as 'www.hello.eu/paths/are/not/allowed'
     *        Must be either a domain name or an IP address with or without an explicit port. 80'th port is used by default.
     * @param ChannelCredentials
     *        Credentials to use for the created RPC client.
     * @param Outer
     *        An outer object, None (or empty) is valid as well, if so, GetTransientPackage() will be used to retrieve a static outer.
     * @return A newly created instance of RPC Client.
     */
    UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category="Vizor|RPC Client", meta=(DisplayName="Create RPC Client", DeterminesOutputType="Class"))
    static URpcClient* CreateRpcClientUri(TSubclassOf<URpcClient> Class, const FString& URI, UChannelCredentials* ChannelCredentials, UObject* Outer = nullptr);

    /**
     * Called when did received any kind of error.
     */
    UPROPERTY(BlueprintAssignable, Category="Vizor|RPC Client", meta=(DisplayName="Event RPC Error"))
    FRpcErrorSignature EventRpcError;

protected:
    /** A pointer to an inner RpcClientWorker sending and receiving messages */
    TUniquePtr<RpcClientWorker> InnerWorker;

private:
    virtual void BeginDestroy() override;

    /** Whether the RPC Client could send requests or not */
    bool bCanSendRequests = false;

    /** A thread, where RPC client worker will reside */
	TAtomic<FRunnableThread*> Thread = { nullptr };

    /** An accumulator for error messages */
    TQueue<FRpcError> ErrorMessageQueue;

    /**
     * Global engine ticker handler
     * Only "IsValid() -> true" if this RPC client "CanSendRequests() -> true"
     */
    FDelegateHandle TickDelegateHandle;
};

template <class T>
FORCEINLINE T* NewRpcClient(const FString& URI, UChannelCredentials* ChannelCredentials, UObject* Outer = nullptr)
{
    static_assert(TIsDerivedFrom<T, URpcClient>::IsDerived, "T must derive URpcClient");
    static_assert(!TIsSame<T, URpcClient>::Value, "T must derive URpcClient, but mustn't be a bare URpcClient");

    return Cast<T>(URpcClient::CreateRpcClientUri(T::StaticClass(), URI, ChannelCredentials, Outer));
}
