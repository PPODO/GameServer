#pragma once
#include <iostream>

enum class EMessageFailedReason : uint8_t {
	E_None,
	E_WrongUserInfo,
	E_ExistUserInfo,
	E_ServerError
};

enum class EMessageResultProtocol : uint8_t {
	E_Failed,
	E_Succeeded
};

enum class EAccountMeesageProtocol : uint8_t {
	E_None,
	E_TrySignIn,
	E_TrySignUp,
	E_TryInfoInquiry,
	E_InfoInquiryResult,
	E_SignInResult,
	E_SignUpResult
};

namespace util {
	static uint32_t MakeMessageProtocol(const uint8_t iMessageProcotol, const uint8_t iResultProcotol, const uint8_t iFailedReason) {
		return (iMessageProcotol | (iResultProcotol << 8) | (iFailedReason << 16));
	}

	static void ExtractMessageProtocol(const uint32_t iMessageProtocol, uint8_t& iOutMessageProtocol, uint8_t& iOutResultProtocol, uint8_t& iOutFailedReason) {
		iOutMessageProtocol = (iMessageProtocol) & 0b1111;
		iOutResultProtocol = (iMessageProtocol >> 8) | 0b0;
		iOutFailedReason = (iMessageProtocol >> 16) | 0b0;
	}
}