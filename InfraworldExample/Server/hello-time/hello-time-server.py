#
# Copyright 2018 Vizor Games LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.See the
# License for the specific language governing permissions and limitations
# under the License.
#
import grpc
from concurrent import futures
import time
from time import strftime
from time import gmtime
import vizor_demonstration_pb2
import vizor_demonstration_pb2_grpc


class VizorDemoServer(vizor_demonstration_pb2_grpc.HelloServiceServicer):
    def Hello(self, request: vizor_demonstration_pb2.HelloRequest, context):
        t = strftime("%H:%M:%S", gmtime())
        print(f'Hello request received from {request.name} {t}!')
        return vizor_demonstration_pb2.HelloResponse(message=f'Vizor Demo Server Greetings you, {request.name}!')

    def ServerTime(self, request, context):
        print('Someone want to know server time...')
        hours = int(strftime('%H'))
        minutes = int(strftime('%M'))
        seconds = int(strftime('%S'))
        timezone = strftime('%z')
        location = 'Europe/Moscow'
        return vizor_demonstration_pb2.ServerTimeResponse(hours=hours, minutes=minutes,
                                                          seconds=seconds, timezone=timezone, location=location)


def start_server():
    print('Server Started!')
    server = grpc.server(futures.ThreadPoolExecutor(max_workers=10))
    vizor_demonstration_pb2_grpc.add_HelloServiceServicer_to_server(VizorDemoServer(), server)
    server.add_insecure_port('[::]:50051')
    server.start()

    try:
        while True:
            time.sleep(60 * 60 * 24)
    except KeyboardInterrupt:
        server.stop(0)


if __name__ == '__main__':
    start_server()
