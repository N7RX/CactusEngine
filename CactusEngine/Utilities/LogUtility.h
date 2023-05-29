#pragma once
#include "Configuration.h"

#include <assert.h>
#include <string>
// Log system has non-negligible performance cost, use logging sparingly
// TODO: add support to write to log file(s) and handle multithreading

namespace Engine
{
	class LogUtility
	{
	public:
		LogUtility();
		~LogUtility() = default;

		void Initialize();
		void ShutDown();

		void LogMessage(const char* message);
		void LogMessage(const std::string& message);

		void LogWarning(const char* warning);
		void LogWarning(const std::string& warning);

		void LogError(const char* error);
		void LogError(const std::string& error);
	};

	extern LogUtility gLogUtilityInstance;

#define LOG_SYSTEM_INITIALIZE() gLogUtilityInstance.Initialize();
#define LOG_SYSTEM_SHUTDOWN() gLogUtilityInstance.ShutDown();

#if defined(DEBUG_MODE_CE)
#define DEBUG_LOG_MESSAGE(message) gLogUtilityInstance.LogMessage(message);
#define DEBUG_LOG_WARNING(warning) gLogUtilityInstance.LogWarning(warning);
#define DEBUG_LOG_ERROR(error) gLogUtilityInstance.LogError(error);

#define DEBUG_ASSERT_CE(condition) assert(condition);
#define DEBUG_ASSERT_MESSAGE_CE(condition, message) { \
	if (!(condition)) \
	{ \
		gLogUtilityInstance.LogError(message); \
		assert(false); \
	} \
}
#else
#define DEBUG_LOG_MESSAGE(message)
#define DEBUG_LOG_WARNING(warning)
#define DEBUG_LOG_ERROR(error)

#define DEBUG_ASSERT_CE(condition)
#define DEBUG_ASSERT_MESSAGE_CE(condition, message)
#endif // _DEBUG

#define LOG_MESSAGE(message) gLogUtilityInstance.LogMessage(message);
#define LOG_WARNING(warning) gLogUtilityInstance.LogWarning(warning);
#define LOG_ERROR(error) gLogUtilityInstance.LogError(error);

#define ASSERT_CE(condition) assert(condition);
#define ASSERT_MESSAGE_CE(condition, message) { \
	if (!(condition)) \
	{ \
		gLogUtilityInstance.LogError(message); \
		assert(false); \
	} \
}
}