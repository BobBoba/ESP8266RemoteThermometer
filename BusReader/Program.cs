﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.ServiceBus.Messaging;
using System.Threading;

namespace BusReader
{
   class Program
   {
      static string connectionString = "HostName=cheluha.azure-devices.net;SharedAccessKeyName=iothubowner;SharedAccessKey=UmD6D/YuggKNVRT0Otqu8Ti2ssnu9jrWes216bFy/V8=";
      static string iotHubD2cEndpoint = "messages/events";
      static EventHubClient eventHubClient;

      private static async Task ReceiveMessagesFromDeviceAsync(string partition, CancellationToken ct)
      {
         var eventHubReceiver = eventHubClient.GetDefaultConsumerGroup().CreateReceiver(partition, DateTime.UtcNow);
         while(true)
         {
            if(ct.IsCancellationRequested)
               break;
            EventData eventData = await eventHubReceiver.ReceiveAsync();
            if(eventData == null)
               continue;

            string data = Encoding.UTF8.GetString(eventData.GetBytes());
            Console.WriteLine("Message received. Partition: {0} Data: '{1}'", partition, data);
         }
      }

      static void Main(string[] args)
      {
         Console.WriteLine("Получение сообщений. Ctrl-C для выхода.\n");
         eventHubClient = EventHubClient.CreateFromConnectionString(connectionString, iotHubD2cEndpoint);

         var d2cPartitions = eventHubClient.GetRuntimeInformation().PartitionIds;

         CancellationTokenSource cts = new CancellationTokenSource();

         System.Console.CancelKeyPress += (s, e) =>
         {
            e.Cancel = true;
            cts.Cancel();
            Console.WriteLine("Выходим...");
         };

         var tasks = new List<Task>();
         foreach(string partition in d2cPartitions)
         {
            tasks.Add(ReceiveMessagesFromDeviceAsync(partition, cts.Token));
         }
         Task.WaitAll(tasks.ToArray());
      }
   }
}
