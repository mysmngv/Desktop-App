#pragma once

#include <QString>
#include <QVariant>

// Basic enumerations
// When adding a new enum, increase the number. Do not delete unused enums, just mark it as deprecated, since they are used in serialization

enum PROXY_OPTION {
    PROXY_OPTION_NONE = 0,
    PROXY_OPTION_AUTODETECT = 1,
    PROXY_OPTION_HTTP = 2,
    PROXY_OPTION_SOCKS = 3
};

enum INIT_HELPER_RET {
    INIT_HELPER_SUCCESS = 0,
    INIT_HELPER_FAILED = 1,
    INIT_HELPER_USER_CANCELED =2
};

enum LOGIN_RET {
    LOGIN_RET_SUCCESS = 0,
    LOGIN_RET_NO_API_CONNECTIVITY = 1,
    LOGIN_RET_NO_CONNECTIVITY = 2,
    LOGIN_RET_INCORRECT_JSON = 3,
    LOGIN_RET_BAD_USERNAME = 4,
    LOGIN_RET_PROXY_AUTH_NEED = 5,
    LOGIN_RET_SSL_ERROR = 6,
    LOGIN_RET_BAD_CODE2FA = 7,
    LOGIN_RET_MISSING_CODE2FA = 8,
    LOGIN_RET_ACCOUNT_DISABLED = 9,
    LOGIN_RET_SESSION_INVALID = 10,
    LOGIN_RET_RATE_LIMITED = 11,
};

enum LOGIN_MESSAGE {
    LOGIN_MESSAGE_NONE = 0,
    LOGIN_MESSAGE_TRYING_BACKUP1 = 1,
    LOGIN_MESSAGE_TRYING_BACKUP2 = 2
};

enum SERVER_API_RET_CODE { SERVER_RETURN_SUCCESS, SERVER_RETURN_NETWORK_ERROR, SERVER_RETURN_NO_NETWORK_CONNECTION, SERVER_RETURN_INCORRECT_JSON,
                           SERVER_RETURN_FAILOVER_FAILED };

enum ENGINE_INIT_RET_CODE { ENGINE_INIT_SUCCESS, ENGINE_INIT_HELPER_FAILED, ENGINE_INIT_BFE_SERVICE_FAILED, ENGINE_INIT_HELPER_USER_CANCELED };

enum CONNECT_STATE {
    CONNECT_STATE_DISCONNECTED = 0,
    CONNECT_STATE_CONNECTED = 1,
    CONNECT_STATE_CONNECTING = 2,
    CONNECT_STATE_DISCONNECTING = 3
};

enum DISCONNECT_REASON {
    DISCONNECTED_ITSELF = 0,
    DISCONNECTED_BY_USER = 1,
    DISCONNECTED_BY_RECONNECTION_TIMEOUT_EXCEEDED = 2,
    DISCONNECTED_WITH_ERROR = 3
};

enum CONNECT_ERROR  {
  NO_CONNECT_ERROR = 0,
  AUTH_ERROR = 1,
  COULD_NOT_FETCH_CREDENTAILS = 2,
  LOCATION_NOT_EXIST = 3,
  LOCATION_NO_ACTIVE_NODES = 4,
  CANT_RESOLVE_HOSTNAME = 5,
  CONNECTION_BLOCKED = 6,
  NO_OPENVPN_SOCKET = 7,
  CANT_RUN_OPENVPN = 8,
  CANNOT_ALLOCATE_TUN_TAP = 9,
  NO_INSTALLED_TUN_TAP = 10,
  ALL_TAP_IN_USE = 11,
  CANNOT_CONNECT_TO_SERVICE_PIPE = 12,
  CANNOT_WRITE_TO_SERVICE_PIPE = 13,
  NO_AVAILABLE_PORT = 14,
  PROXY_AUTH_ERROR = 15,
  UDP_CANT_ASSIGN = 16,
  CONNECTED_ERROR = 17,
  INITIALIZATION_SEQUENCE_COMPLETED_WITH_ERRORS = 18,
  UDP_NO_BUFFER_SPACE = 19,
  UDP_NETWORK_DOWN = 20,
  TCP_ERROR = 21,
  CANNOT_OPEN_CUSTOM_CONFIG = 22,
  IKEV_FAILED_TO_CONNECT = 23,
  IKEV_NOT_FOUND_WIN = 24,
  IKEV_FAILED_SET_ENTRY_WIN = 25,
  IKEV_FAILED_MODIFY_HOSTS_WIN = 26,
  IKEV_NETWORK_EXTENSION_NOT_FOUND_MAC = 27,
  IKEV_FAILED_SET_KEYCHAIN_MAC = 28,
  IKEV_FAILED_START_MAC = 29,
  IKEV_FAILED_LOAD_PREFERENCES_MAC = 30,
  IKEV_FAILED_SAVE_PREFERENCES_MAC = 31,
  WIREGUARD_CONNECTION_ERROR = 32,
  EMERGENCY_FAILED_CONNECT = 33,
  WINTUN_OVER_CAPACITY = 34,
  WINTUN_FATAL_ERROR = 35,
  EXE_VERIFY_WSTUNNEL_ERROR = 36,
  EXE_VERIFY_STUNNEL_ERROR = 37,
  EXE_VERIFY_OPENVPN_ERROR = 38,
  EXE_VERIFY_WIREGUARD_ERROR = 39,
  STATE_TIMEOUT_FOR_AUTOMATIC = 40,
  WIREGUARD_ADAPTER_SETUP_FAILED = 41,
  CTRLD_START_FAILED = 42
};

enum PROXY_SHARING_TYPE {
    PROXY_SHARING_HTTP = 0,
    PROXY_SHARING_SOCKS = 1
};

enum ORDER_LOCATIONS_TYPE {
    ORDER_LOCATIONS_BY_GEOGRAPHY = 0,
    ORDER_LOCATIONS_BY_ALPHABETICALLY = 1,
    ORDER_LOCATIONS_BY_LATENCY = 2
};

enum DNS_POLICY_TYPE {
    DNS_TYPE_OS_DEFAULT = 0,
    DNS_TYPE_OPEN_DNS = 1,
    DNS_TYPE_CLOUDFLARE = 2,
    DNS_TYPE_GOOGLE = 3,
    DNS_TYPE_CONTROLD = 4
};

enum UPDATE_CHANNEL {
    UPDATE_CHANNEL_RELEASE = 0,
    UPDATE_CHANNEL_BETA = 1,
    UPDATE_CHANNEL_GUINEA_PIG = 2,
    UPDATE_CHANNEL_INTERNAL = 3
};

enum FIREWALL_MODE {
    FIREWALL_MODE_MANUAL = 0,
    FIREWALL_MODE_AUTOMATIC = 1,
    FIREWALL_MODE_ALWAYS_ON =2
};

enum FIREWALL_WHEN {
    FIREWALL_WHEN_BEFORE_CONNECTION = 0,
    FIREWALL_WHEN_AFTER_CONNECTION = 1
};

enum NETWORK_INTERACE_TYPE {
    NETWORK_INTERFACE_NONE = 0,
    NETWORK_INTERFACE_ETH = 1,
    NETWORK_INTERFACE_WIFI = 2,
    NETWORK_INTERFACE_PPP = 3
};

enum NETWORK_TRUST_TYPE {
    NETWORK_TRUST_SECURED = 0,
    NETWORK_TRUST_UNSECURED = 1,
    NETWORK_TRUST_FORGET = 2
};

enum TAP_ADAPTER_TYPE {
    DCO_ADAPTER = 1,
    WINTUN_ADAPTER = 2
};

enum CONNECTED_DNS_TYPE {
    CONNECTED_DNS_TYPE_ROBERT = 0,
    CONNECTED_DNS_TYPE_CUSTOM = 1
};

enum DNS_MANAGER_TYPE {
    DNS_MANAGER_AUTOMATIC = 0,
    DNS_MANAGER_RESOLV_CONF = 1,
    DNS_MANAGER_SYSTEMD_RESOLVED = 2,
    DNS_MANAGER_NETWORK_MANAGER = 3
};

enum USER_WARNING_TYPE {
    USER_WARNING_MAC_SPOOFING_FAILURE_HARD = 0,
    USER_WARNING_MAC_SPOOFING_FAILURE_SOFT = 1,
    USER_WARNING_CHECK_UPDATE_INVALID_PLATFORM = 4
};

enum INIT_STATE {
    INIT_STATE_CLEAN = 0,
    INIT_STATE_SUCCESS = 1,
    INIT_STATE_HELPER_FAILED = 2,
    INIT_STATE_HELPER_USER_CANCELED = 3,
    INIT_STATE_BFE_SERVICE_NOT_STARTED = 4,
    INIT_STATE_BFE_SERVICE_FAILED_TO_START = 5
};

enum WEB_SESSION_PURPOSE {
    WEB_SESSION_PURPOSE_MANAGE_ACCOUNT = 0,
    WEB_SESSION_PURPOSE_ADD_EMAIL = 1,
    WEB_SESSION_PURPOSE_MANAGE_ROBERT_RULES = 2
};

enum CUSTOM_CONFIG_TYPE {
    CUSTOM_CONFIG_OPENVPN = 0,
    CUSTOM_CONFIG_WIREGUARD = 1
};

enum SPLIT_TUNNELING_MODE {
    SPLIT_TUNNELING_MODE_EXCLUDE = 0,
    SPLIT_TUNNELING_MODE_INCLUDE = 1
};

enum SPLIT_TUNNELING_APP_TYPE {
    SPLIT_TUNNELING_APP_TYPE_USER = 0,
    SPLIT_TUNNELING_APP_TYPE_SYSTEM = 1
};

enum SPLIT_TUNNELING_NETWORK_ROUTE_TYPE {
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_IP = 0,
    SPLIT_TUNNELING_NETWORK_ROUTE_TYPE_HOSTNAME = 1
};


enum ORDER_LOCATION_TYPE {
    ORDER_LOCATION_BY_GEOGRAPHY = 0,
    ORDER_LOCATION_BY_ALPHABETICALLY = 1,
    ORDER_LOCATION_BY_LATENCY = 2
};

enum LATENCY_DISPLAY_TYPE {
    LATENCY_DISPLAY_BARS = 0,
    LATENCY_DISPLAY_MS = 1
};

enum BACKGROUND_TYPE
{
    BACKGROUND_TYPE_NONE = 0,
    BACKGROUND_TYPE_COUNTRY_FLAGS = 1,
    BACKGROUND_TYPE_CUSTOM = 2
};

enum UPDATE_VERSION_STATE {
    UPDATE_VERSION_STATE_INIT = 0,
    UPDATE_VERSION_STATE_DOWNLOADING = 1,
    UPDATE_VERSION_STATE_RUNNING = 2,
    UPDATE_VERSION_STATE_DONE = 3
};

enum UPDATE_VERSION_ERROR
{
    UPDATE_VERSION_ERROR_NO_ERROR = 0,
    UPDATE_VERSION_ERROR_DL_FAIL = 1,
    UPDATE_VERSION_ERROR_SIGN_FAIL = 2,
    UPDATE_VERSION_ERROR_OTHER_FAIL = 3,
    UPDATE_VERSION_ERROR_MOUNT_FAIL = 4,
    UPDATE_VERSION_ERROR_DMG_HAS_NO_INSTALLER_FAIL = 5,
    UPDATE_VERSION_ERROR_CANNOT_REMOVE_EXISTING_TEMP_INSTALLER_FAIL = 6,
    UPDATE_VERSION_ERROR_COPY_FAIL = 7,
    UPDATE_VERSION_ERROR_START_INSTALLER_FAIL = 8,
    UPDATE_VERSION_ERROR_COMPARE_HASH_FAIL = 9,
    UPDATE_VERSION_ERROR_API_HASH_INVALID = 10
};

enum APP_SKIN
{
    APP_SKIN_ALPHA = 0,
    APP_SKIN_VAN_GOGH = 1
};

enum TRAY_ICON_COLOR
{
    TRAY_ICON_COLOR_WHITE = 0,
    TRAY_ICON_COLOR_BLACK = 1
};

enum LOCATION_TAB {
    LOCATION_TAB_NONE = 0,
    LOCATION_TAB_ALL_LOCATIONS,
    LOCATION_TAB_FAVORITE_LOCATIONS,
    LOCATION_TAB_STATIC_IPS_LOCATIONS,
    LOCATION_TAB_CONFIGURED_LOCATIONS,
    LOCATION_TAB_SEARCH_LOCATIONS,
    LOCATION_TAB_FIRST = LOCATION_TAB_ALL_LOCATIONS,
    LOCATION_TAB_LAST = LOCATION_TAB_SEARCH_LOCATIONS
};

// utils for enums
QString LOGIN_RET_toString(LOGIN_RET ret);
QString DNS_POLICY_TYPE_ToString(DNS_POLICY_TYPE d);
QList<QPair<QString, QVariant>> DNS_POLICY_TYPE_toList();

QString CONNECTED_DNS_TYPE_toString(CONNECTED_DNS_TYPE t);
QString SPLIT_TUNNELING_MODE_toString(SPLIT_TUNNELING_MODE t);

QString PROXY_SHARING_TYPE_toString(PROXY_SHARING_TYPE t);
QList<QPair<QString, QVariant>> PROXY_SHARING_TYPE_toList();

QString ORDER_LOCATION_TYPE_toString(ORDER_LOCATION_TYPE p);
QList<QPair<QString, QVariant>> ORDER_LOCATION_TYPE_toList();

QString LATENCY_DISPLAY_TYPE_toString(LATENCY_DISPLAY_TYPE t);
QList<QPair<QString, QVariant>> LATENCY_DISPLAY_TYPE_toList();

QString TAP_ADAPTER_TYPE_toString(TAP_ADAPTER_TYPE t);

QString FIREWALL_MODE_toString(FIREWALL_MODE t);
QList<QPair<QString, QVariant>> FIREWALL_MODE_toList();

QString FIREWALL_WHEN_toString(FIREWALL_WHEN t);
QList<QPair<QString, QVariant>> FIREWALL_WHEN_toList();

QString PROXY_OPTION_toString(PROXY_OPTION t);
QList<QPair<QString, QVariant>> PROXY_OPTION_toList();

QString UPDATE_CHANNEL_toString(UPDATE_CHANNEL t);
QList<QPair<QString, QVariant>> UPDATE_CHANNEL_toList();

QString DNS_MANAGER_TYPE_toString(DNS_MANAGER_TYPE t);
QList<QPair<QString, QVariant>> DNS_MANAGER_TYPE_toList();

QString APP_SKIN_toString(APP_SKIN s);
QList<QPair<QString, QVariant>> APP_SKIN_toList();

#if defined(Q_OS_LINUX)
QString TRAY_ICON_COLOR_toString(TRAY_ICON_COLOR c);
QList<QPair<QString, QVariant>> TRAY_ICON_COLOR_toList();
#endif
