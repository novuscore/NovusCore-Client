/*
# MIT License

# Copyright(c) 2018-2019 NovusCore

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files(the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions :

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
*/
#pragma once

enum AuthCommand
{
    AUTH_CHALLENGE = 0x00,
    AUTH_PROOF = 0x01,
    AUTH_RECONNECT_CHALLENGE = 0x02,
    AUTH_RECONNECT_PROOF = 0x03,
    AUTH_REALMSERVER_LIST = 0x10,
};
enum AuthStatus
{
    STATUS_CHALLENGE = 0,
    STATUS_PROOF = 1,
    STATUS_RECONNECT_PROOF = 2,
    STATUS_AUTHED = 3,
    STATUS_WAITING_FOR_REALMSERVER_LIST = 4,
    STATUS_CLOSED = 5
};

enum AuthResult
{
    AUTH_SUCCESS = 0x00,
    AUTH_FAIL_BANNED = 0x03,
    AUTH_FAIL_UNKNOWN_ACCOUNT = 0x04,
    AUTH_FAIL_INCORRECT_PASSWORD = 0x05,
    AUTH_FAIL_ALREADY_ONLINE = 0x06,
    AUTH_FAIL_NO_TIME = 0x07,
    AUTH_FAIL_DB_BUSY = 0x08,
    AUTH_FAIL_VERSION_INVALID = 0x09,
    AUTH_FAIL_VERSION_UPDATE = 0x0A,
    AUTH_FAIL_INVALID_SERVER = 0x0B,
    AUTH_FAIL_SUSPENDED = 0x0C,
    AUTH_FAIL_FAIL_NOACCESS = 0x0D,
    AUTH_SUCCESS_SURVEY = 0x0E,
    AUTH_FAIL_PARENTCONTROL = 0x0F,
    AUTH_FAIL_LOCKED_ENFORCED = 0x10,
    AUTH_FAIL_TRIAL_ENDED = 0x11,
    AUTH_FAIL_USE_BATTLENET = 0x12,
    AUTH_FAIL_ANTI_INDULGENCE = 0x13,
    AUTH_FAIL_EXPIRED = 0x14,
    AUTH_FAIL_NO_GAME_ACCOUNT = 0x15,
    AUTH_FAIL_CHARGEBACK = 0x16,
    AUTH_FAIL_INTERNET_GAME_ROOM_WITHOUT_BNET = 0x17,
    AUTH_FAIL_GAME_ACCOUNT_LOCKED = 0x18,
    AUTH_FAIL_UNLOCKABLE_LOCK = 0x19,
    AUTH_FAIL_CONVERSION_REQUIRED = 0x20,
    AUTH_FAIL_DISCONNECTED = 0xFF
};

enum LoginResult
{
    LOGIN_OK = 0x00,
    LOGIN_FAILED = 0x01,
    LOGIN_FAILED2 = 0x02,
    LOGIN_BANNED = 0x03,
    LOGIN_UNKNOWN_ACCOUNT = 0x04,
    LOGIN_UNKNOWN_ACCOUNT3 = 0x05,
    LOGIN_ALREADYONLINE = 0x06,
    LOGIN_NOTIME = 0x07,
    LOGIN_DBBUSY = 0x08,
    LOGIN_BADVERSION = 0x09,
    LOGIN_DOWNLOAD_FILE = 0x0A,
    LOGIN_FAILED3 = 0x0B,
    LOGIN_SUSPENDED = 0x0C,
    LOGIN_FAILED4 = 0x0D,
    LOGIN_CONNECTED = 0x0E,
    LOGIN_PARENTALCONTROL = 0x0F,
    LOGIN_LOCKED_ENFORCED = 0x10
};