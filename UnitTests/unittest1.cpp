#include "stdafx.h"
#include "CppUnitTest.h"

#include <ArduinoJson.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace UnitTests
{		
	TEST_CLASS(UnitTest1)
	{
	public:
		
		TEST_METHOD(TestMethod1)
		{
			StaticJsonBuffer<200> jsonBuffer;
			JsonObject& root = jsonBuffer.createObject();

			root["MAC"] = "12354678";

			JsonObject& jo = root.createNestedObject("nestedObject");

			jo["f"] = "d";

			JsonArray& ja = root.createNestedArray("nestedArray");

			//ja.add("data");


			char buffer[1024] = { 0 };
			root.prettyPrintTo(buffer);
			printf(buffer);
			Logger::WriteMessage(buffer);
		}

	};
}