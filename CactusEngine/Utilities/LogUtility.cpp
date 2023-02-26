#include "LogUtility.h"

#include <iostream>

namespace Engine
{
	LogUtility gLogUtilityInstance;

	LogUtility::LogUtility()
	{

	}

	void LogUtility::Initialize()
	{

	}

	void LogUtility::ShutDown()
	{

	}

	void LogUtility::LogMessage(const char* message)
	{
		std::cout << message << std::endl;
	}

	void LogUtility::LogMessage(const std::string& message)
	{
		std::cout << message << std::endl;
	}

	void LogUtility::LogWarning(const char* warning)
	{
		std::cout << "WARNING: " << warning << std::endl;
	}

	void LogUtility::LogWarning(const std::string& warning)
	{
		std::cout << "WARNING: " << warning << std::endl;
	}

	void LogUtility::LogError(const char* error)
	{
		std::cerr << "[ERROR]: " << error << std::endl;
	}

	void LogUtility::LogError(const std::string& error)
	{
		std::cerr << "[ERROR]: " << error << std::endl;
	}
}