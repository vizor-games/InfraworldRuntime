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

class FGrpcUriValidator
{
public:
    /**
     * Attempts to validate a URI, further provided to grpc::CreateChannel function.
     * It does not tries to establish any kind of connections, so it checks only format.
     *
     * @param MaybeGrpcUri Grpc URI to validate.
     * @param OutError Error message if any.
     *
     * @return True if the URI is valid and thus could be used, false otherwise.
     */
    static bool Validate(const FString& MaybeGrpcUri, FString& OutError);
};
