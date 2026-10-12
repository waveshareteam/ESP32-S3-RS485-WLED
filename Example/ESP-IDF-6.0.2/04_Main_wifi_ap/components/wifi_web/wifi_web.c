#include "wifi_web.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "audio_service.h"
#include "esp_check.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "light_service.h"
#include "rs485_service.h"

static const char *TAG = "wifi_web";

#define AP_SSID      "ESP32-S3-RS485-WLED"
#define AP_PASSWORD  "12345678"
#define AP_CHANNEL   1
#define AP_MAX_CONN  4
#define RS485_HTTP_DATA_SIZE 4096
#define RS485_HTTP_JSON_SIZE 8192

static httpd_handle_t s_server;

static const char s_index_html[] =
"<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
"<title>ESP32-S3-RS485-WLED</title>"
"<style>body{font-family:Arial,sans-serif;background:#f2f4f8;margin:0;color:#1f2937}.header{background:#1f2937;color:#fff;text-align:center;padding:18px}.container{max-width:720px;margin:18px auto;padding:18px;background:#fff;border-radius:6px;box-shadow:0 1px 8px #0002}label{display:block;font-weight:bold;margin:12px 0 6px}input,select,textarea,button{font-size:14px}input,select,textarea{box-sizing:border-box;width:100%;padding:8px;border:1px solid #cbd5e1;border-radius:4px}textarea{height:150px;resize:vertical;font-family:Consolas,monospace}.row{display:grid;grid-template-columns:1fr 1fr;gap:12px}.light-row{display:grid;grid-template-columns:24px 1fr 42px;gap:8px;align-items:center;margin:8px 0}input[type=range]{--pct:0%;appearance:none;-webkit-appearance:none;border:0;padding:0;height:24px;background:transparent}input[type=range]::-webkit-slider-runnable-track{height:6px;border-radius:3px;background:linear-gradient(to right,#1677ff 0 var(--pct),#e5e7eb var(--pct) 100%);border:1px solid #b8c0cc}input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:16px;height:16px;border-radius:50%;background:#1677ff;border:0;margin-top:-6px}input[type=range]::-moz-range-track{height:6px;border-radius:3px;background:#e5e7eb;border:1px solid #b8c0cc}input[type=range]::-moz-range-progress{height:6px;border-radius:3px;background:#1677ff}input[type=range]::-moz-range-thumb{width:16px;height:16px;border-radius:50%;background:#1677ff;border:0}button{margin-top:10px;padding:8px 12px;background:#1f2937;color:white;border:0;border-radius:4px;cursor:pointer}.status{font-size:12px;color:#64748b;margin-top:8px;min-height:18px}@media(max-width:680px){.container{margin:0;border-radius:0}.row{grid-template-columns:1fr}}</style>"
"</head><body><div class=\"header\"><h1>ESP32-S3-RS485-WLED</h1></div><div class=\"container\">"
"<h3>RS485</h3><div class=\"row\"><div><label>Baud Rate</label><select id=\"baud\"><option>110</option><option>300</option><option>600</option><option>1200</option><option>2400</option><option>4800</option><option selected>9600</option><option>14400</option><option>19200</option><option>38400</option><option>56000</option><option>57600</option><option>115200</option><option>128000</option><option>230400</option><option>460800</option><option>500000</option><option>512000</option><option>600000</option><option>750000</option><option>921600</option><option>1000000</option><option>1500000</option><option>2000000</option><option>3000000</option><option>5000000</option></select><button onclick=\"setBaud()\">Set Baud Rate</button></div>"
"<div><label>Receive Format</label><select id=\"rxType\"><option value=\"0\">char</option><option value=\"1\">hex</option></select><button onclick=\"setRxType()\">Set Format</button></div></div>"
"<label>Send Format</label><select id=\"txType\"><option value=\"0\">char</option><option value=\"1\">hex</option></select><label>RS485 Send Data</label><input id=\"txData\" value=\"12\" oninput=\"formatHexInput(this)\"><button onclick=\"sendRs485()\">Send Data</button><div class=\"status\" id=\"rs485Status\">Ready</div><label>RS485 Receive Data</label><textarea id=\"rxData\" placeholder=\"No data received...\"></textarea>"
"<h3>Audio</h3><button onclick=\"audioReq('/AudioPlayMusic')\">Play / Pause Music</button> <button onclick=\"audioReq('/AudioRecord5s')\">Record 5s + Play</button><div class=\"status\" id=\"audioStatus\">Audio ready</div>"
"<h3>Light</h3><div class=\"row\"><div><label>GPIO</label><input id=\"lightGpio\" type=\"number\" value=\"2\" min=\"0\" max=\"48\"></div><div><label>Count</label><input id=\"lightCount\" type=\"number\" value=\"160\" min=\"1\" max=\"2048\"></div></div><button onclick=\"lightConfig()\">Set Light Config</button>"
"<div class=\"light-row\"><span>R</span><input id=\"lightR\" type=\"range\" min=\"0\" max=\"255\" value=\"0\" oninput=\"labels()\" onchange=\"lightColor()\"><span id=\"lightRValue\">0</span></div><div class=\"light-row\"><span>G</span><input id=\"lightG\" type=\"range\" min=\"0\" max=\"255\" value=\"0\" oninput=\"labels()\" onchange=\"lightColor()\"><span id=\"lightGValue\">0</span></div><div class=\"light-row\"><span>B</span><input id=\"lightB\" type=\"range\" min=\"0\" max=\"255\" value=\"0\" oninput=\"labels()\" onchange=\"lightColor()\"><span id=\"lightBValue\">0</span></div><button onclick=\"lightColor()\">Apply Color</button> <button onclick=\"lightOff()\">Off</button><div class=\"status\" id=\"lightStatus\">Light ready</div>"
"</div><script>"
"function q(id){return document.getElementById(id)}function req(p,cb){var x=new XMLHttpRequest();x.open('GET',p,true);x.onreadystatechange=function(){if(x.readyState===4&&cb)cb(x)};x.send()}function formatHexInput(i){if(q('txType').value!=='1')return;var r=i.value.replace(/[^0-9a-fA-F]/g,'');var p=r.match(/.{1,2}/g);i.value=p?p.join(' '):''}"
"function readCfg(){req('/getRateConfig',function(x){if(x.status===200){var d=JSON.parse(x.responseText);q('baud').value=d.rs485_baud}})}function setBaud(){req('/RS485SetBaudRate?baud='+encodeURIComponent(q('baud').value),function(x){q('rs485Status').innerText=x.status===200?'Baud updated':'Baud failed'})}function setRxType(){req('/RS485SetConfig?type='+encodeURIComponent(q('rxType').value),function(x){q('rs485Status').innerText=x.status===200?'Format updated':'Format failed'})}function sendRs485(){var p='/RS485Send?type='+encodeURIComponent(q('txType').value)+'&data='+encodeURIComponent(q('txData').value);req(p,function(x){q('rs485Status').innerText=x.status===200?'Data sent':'Send failed'})}"
"function readRs485(){req('/getRS485Data',function(x){if(x.status===200){var d=JSON.parse(x.responseText);if(d.length&&d[0]){var t=q('rxData');var b=(t.scrollHeight-t.scrollTop-t.clientHeight)<10;t.value+=d[0];if(b)t.scrollTop=t.scrollHeight}}})}"
"function audioReq(p){req(p,function(x){if(x.status===200){try{q('audioStatus').innerText=JSON.parse(x.responseText).status}catch(e){q('audioStatus').innerText=x.responseText}}else q('audioStatus').innerText='Audio request failed'})}"
"function labels(){['R','G','B'].forEach(function(n){var e=q('light'+n);var v=Number(e.value)||0;e.style.setProperty('--pct',(v*100/255)+'%');q('light'+n+'Value').innerText=e.value})}function lightReq(p){req(p,function(x){if(x.status===200){var d=JSON.parse(x.responseText);q('lightStatus').innerText=d.status||'OK';q('lightGpio').value=d.gpio;q('lightCount').value=d.count;q('lightR').value=d.r;q('lightG').value=d.g;q('lightB').value=d.b;labels()}else q('lightStatus').innerText='Light request failed'})}function lightConfig(){lightReq('/LightSetConfig?gpio='+q('lightGpio').value+'&count='+q('lightCount').value)}function lightColor(){lightReq('/LightSetColor?r='+q('lightR').value+'&g='+q('lightG').value+'&b='+q('lightB').value)}function lightOff(){lightReq('/LightOff')}readCfg();labels();setInterval(readRs485,500);setInterval(function(){audioReq('/AudioStatus')},2000);"
"</script></body></html>";

static int from_hex(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static void url_decode(char *s)
{
    char *src = s;
    char *dst = s;
    while (*src) {
        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            *dst++ = (char)((from_hex(src[1]) << 4) | from_hex(src[2]));
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

static bool query_value(httpd_req_t *req, const char *key, char *out, size_t out_size)
{
    size_t query_len = httpd_req_get_url_query_len(req) + 1;
    if (query_len <= 1) {
        return false;
    }

    char *query = malloc(query_len);
    if (!query) {
        return false;
    }

    bool ok = false;
    if (httpd_req_get_url_query_str(req, query, query_len) == ESP_OK &&
        httpd_query_key_value(query, key, out, out_size) == ESP_OK) {
        url_decode(out);
        ok = true;
    }
    free(query);
    return ok;
}

static void json_escape(const char *in, char *out, size_t out_size)
{
    size_t pos = 0;
    for (size_t i = 0; in[i] && pos + 2 < out_size; i++) {
        char c = in[i];
        if (c == '"' || c == '\\') {
            if (pos + 3 >= out_size) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = c;
        } else if (c == '\n') {
            if (pos + 3 >= out_size) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = 'n';
        } else if (c == '\r') {
            if (pos + 3 >= out_size) {
                break;
            }
            out[pos++] = '\\';
            out[pos++] = 'r';
        } else {
            out[pos++] = c;
        }
    }
    out[pos] = '\0';
}

static size_t parse_hex_data(const char *text, uint8_t *out, size_t out_size)
{
    int high = -1;
    size_t len = 0;
    while (*text) {
        int v = from_hex(*text++);
        if (v < 0) {
            continue;
        }
        if (high < 0) {
            high = v;
        } else if (len < out_size) {
            out[len++] = (uint8_t)((high << 4) | v);
            high = -1;
        }
    }
    if (high >= 0 && len < out_size) {
        out[len++] = (uint8_t)(high << 4);
    }
    return len;
}

static esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    return httpd_resp_send(req, s_index_html, HTTPD_RESP_USE_STRLEN);
}

static esp_err_t favicon_handler(httpd_req_t *req)
{
    httpd_resp_set_status(req, "204 No Content");
    return httpd_resp_send(req, NULL, 0);
}

static esp_err_t rate_config_handler(httpd_req_t *req)
{
    char json[64];
    snprintf(json, sizeof(json), "{\"rs485_baud\":\"%lu\"}", (unsigned long)rs485_service_get_baud());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t rs485_data_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");

    char *data = malloc(RS485_HTTP_DATA_SIZE);
    char *escaped = malloc(RS485_HTTP_JSON_SIZE);
    if (!data || !escaped) {
        free(data);
        free(escaped);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "no memory");
        return ESP_ERR_NO_MEM;
    }

    size_t len = rs485_service_take_received(data, RS485_HTTP_DATA_SIZE);
    if (len == 0) {
        free(data);
        free(escaped);
        return httpd_resp_sendstr(req, "[]");
    }

    json_escape(data, escaped, RS485_HTTP_JSON_SIZE);
    httpd_resp_sendstr_chunk(req, "[\"");
    httpd_resp_sendstr_chunk(req, escaped);
    httpd_resp_sendstr_chunk(req, "\"]");
    esp_err_t ret = httpd_resp_sendstr_chunk(req, NULL);
    free(data);
    free(escaped);
    return ret;
}

static esp_err_t rs485_set_baud_handler(httpd_req_t *req)
{
    char value[32];
    if (!query_value(req, "baud", value, sizeof(value))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing baud");
        return ESP_FAIL;
    }
    esp_err_t ret = rs485_service_set_baud((uint32_t)strtoul(value, NULL, 10));
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "invalid baud");
        return ret;
    }
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t rs485_set_config_handler(httpd_req_t *req)
{
    char value[16];
    if (!query_value(req, "type", value, sizeof(value))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing type");
        return ESP_FAIL;
    }
    rs485_service_set_rx_format(atoi(value) ? RS485_FORMAT_HEX : RS485_FORMAT_CHAR);
    return httpd_resp_sendstr(req, "OK");
}

static esp_err_t rs485_send_handler(httpd_req_t *req)
{
    char type_value[16] = "0";
    char data_value[1024];
    uint8_t buffer[512];

    query_value(req, "type", type_value, sizeof(type_value));
    if (!query_value(req, "data", data_value, sizeof(data_value))) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "missing data");
        return ESP_FAIL;
    }

    size_t send_len = 0;
    if (atoi(type_value)) {
        send_len = parse_hex_data(data_value, buffer, sizeof(buffer));
    } else {
        send_len = strnlen(data_value, sizeof(data_value));
        if (send_len > sizeof(buffer)) {
            send_len = sizeof(buffer);
        }
        memcpy(buffer, data_value, send_len);
    }

    esp_err_t ret = rs485_service_send(buffer, send_len);
    if (ret != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "send failed");
        return ret;
    }
    return httpd_resp_sendstr(req, "OK");
}

static uint8_t query_u8(httpd_req_t *req, const char *key)
{
    char value[16];
    return query_value(req, key, value, sizeof(value)) ? (uint8_t)atoi(value) : 0;
}

static esp_err_t light_set_config_handler(httpd_req_t *req)
{
    char value[16];
    uint8_t gpio = LIGHT_DEFAULT_GPIO;
    uint16_t count = LIGHT_DEFAULT_COUNT;
    if (query_value(req, "gpio", value, sizeof(value))) {
        gpio = (uint8_t)atoi(value);
    }
    if (query_value(req, "count", value, sizeof(value))) {
        count = (uint16_t)atoi(value);
    }
    light_service_set_config(gpio, count);
    char json[160];
    light_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t light_set_color_handler(httpd_req_t *req)
{
    light_service_set_color(query_u8(req, "r"), query_u8(req, "g"), query_u8(req, "b"));
    char json[160];
    light_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t light_off_handler(httpd_req_t *req)
{
    light_service_off();
    char json[160];
    light_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t light_status_handler(httpd_req_t *req)
{
    char json[160];
    light_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t audio_play_handler(httpd_req_t *req)
{
    audio_service_toggle_music();
    char json[192];
    audio_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t audio_record_handler(httpd_req_t *req)
{
    audio_service_record_5s_and_play();
    char json[192];
    audio_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t audio_status_handler(httpd_req_t *req)
{
    char json[192];
    audio_service_status_json(json, sizeof(json));
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_sendstr(req, json);
}

static esp_err_t wifi_init_softap(void)
{
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init failed");
    esp_err_t event_ret = esp_event_loop_create_default();
    if (event_ret != ESP_OK && event_ret != ESP_ERR_INVALID_STATE) {
        return event_ret;
    }

    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap();
    ESP_RETURN_ON_FALSE(ap_netif != NULL, ESP_FAIL, TAG, "create AP netif failed");

    esp_netif_ip_info_t ip_info = {
        .ip.addr = ESP_IP4TOADDR(192, 168, 4, 1),
        .gw.addr = ESP_IP4TOADDR(192, 168, 4, 1),
        .netmask.addr = ESP_IP4TOADDR(255, 255, 255, 0),
    };
    esp_netif_dhcps_stop(ap_netif);
    esp_netif_set_ip_info(ap_netif, &ip_info);
    esp_netif_dhcps_start(ap_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init failed");

    wifi_config_t wifi_config = {0};
    strlcpy((char *)wifi_config.ap.ssid, AP_SSID, sizeof(wifi_config.ap.ssid));
    strlcpy((char *)wifi_config.ap.password, AP_PASSWORD, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(AP_SSID);
    wifi_config.ap.channel = AP_CHANNEL;
    wifi_config.ap.max_connection = AP_MAX_CONN;
    wifi_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
    if (strlen(AP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "set AP mode failed");
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "set AP config failed");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start failed");
    ESP_LOGI(TAG, "WiFi AP started: ssid=%s password=%s ip=192.168.4.1", AP_SSID, AP_PASSWORD);
    return ESP_OK;
}

esp_err_t wifi_web_start(void)
{
    ESP_RETURN_ON_ERROR(wifi_init_softap(), TAG, "wifi AP failed");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.stack_size = 12288;
    config.max_uri_handlers = 20;

    ESP_RETURN_ON_ERROR(httpd_start(&s_server, &config), TAG, "http server start failed");

    const httpd_uri_t uris[] = {
        {.uri = "/", .method = HTTP_GET, .handler = root_handler},
        {.uri = "/favicon.ico", .method = HTTP_GET, .handler = favicon_handler},
        {.uri = "/getRateConfig", .method = HTTP_GET, .handler = rate_config_handler},
        {.uri = "/getRS485Data", .method = HTTP_GET, .handler = rs485_data_handler},
        {.uri = "/RS485SetBaudRate", .method = HTTP_GET, .handler = rs485_set_baud_handler},
        {.uri = "/RS485SetConfig", .method = HTTP_GET, .handler = rs485_set_config_handler},
        {.uri = "/RS485Send", .method = HTTP_GET, .handler = rs485_send_handler},
        {.uri = "/LightSetConfig", .method = HTTP_GET, .handler = light_set_config_handler},
        {.uri = "/LightSetColor", .method = HTTP_GET, .handler = light_set_color_handler},
        {.uri = "/LightOff", .method = HTTP_GET, .handler = light_off_handler},
        {.uri = "/LightStatus", .method = HTTP_GET, .handler = light_status_handler},
        {.uri = "/AudioPlayMusic", .method = HTTP_GET, .handler = audio_play_handler},
        {.uri = "/AudioRecord5s", .method = HTTP_GET, .handler = audio_record_handler},
        {.uri = "/AudioStatus", .method = HTTP_GET, .handler = audio_status_handler},
    };

    for (size_t i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        ESP_RETURN_ON_ERROR(httpd_register_uri_handler(s_server, &uris[i]), TAG, "register uri failed");
    }

    ESP_LOGI(TAG, "HTTP server started");
    return ESP_OK;
}
