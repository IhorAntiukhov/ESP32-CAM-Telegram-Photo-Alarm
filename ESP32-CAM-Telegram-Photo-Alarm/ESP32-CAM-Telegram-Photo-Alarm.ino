#include "SPIFFS.h"     // библиотека для работы с файловой системой SPIFFS
#include "esp_camera.h" // библиотека для работы с камерой на плате ESP32-CAM
#include "index.h"      // объявляем Web страницу на HTML и CSS

#include <UniversalTelegramBot.h> // библиотека для управления Telegram ботом
#include <ArduinoJson.h>          // библиотека дополняющая библиотеку "Universal Telegram Bot"

// объявляем библиотеки для детектора отключения питания
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#include "esp_http_server.h" // библиотека для работы с HTTP сервером
#include "img_converters.h"  // библиотека для конвертирования изображения
#include "esp_timer.h"       // библиотека для высокочастотного таймера
#include "Arduino.h"         // библиотека для поддержки других Arduino библиотек
#include "fb_gfx.h"          // библиотека для дополнения камеры

// объявляем HTTP метод для предотвращения конфликта библиотек "esp_http_server.h" и "WebServer.h"
#define _HTTP_Method_H_
// переназначаем HTTP методы для библиотеки "esp_http_server.h"
typedef enum {
  jHTTP_GET     = 0b00000001,
  jHTTP_POST    = 0b00000010,
  jHTTP_DELETE  = 0b00000100,
  jHTTP_PUT     = 0b00001000,
  jHTTP_PATCH   = 0b00010000,
  jHTTP_HEAD    = 0b00100000,
  jHTTP_OPTIONS = 0b01000000,
  jHTTP_ANY     = 0b01111111,
} HTTPMethod;

#include <WiFi.h>             // библиотека для работы с WiFi
#include <WiFiClientSecure.h> // библиотека для безопасного WiFi соединения
#include <WebServer.h>        // библиотека для работы с Web сервером

// объявляем пины к которым подключена камера на ESP32-CAМ AI-Thinker

#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define flash_led_pin 4       // пин к которому подключена вспышка
#define reset_button_pin 12   // пин к которому подключена кнопка сброса параметров работы
#define motion_sensor_pin 13  // пин к которому подключен датчик движения
#define button_hold_time 3000 // время удержания кнопки сброса параметров работы в миллисекундах
#define send_photo_wait 10000 // время ожидания отправки фотографии Telegram пользователю

const char *ssid_name = "ESP32-CAM"; // название WiFi точки на которой будет запущен Web сервер
const char *password  = "12345678";  // пароль от объявленной выше WiFi точки
int connections; // переменная для хранения количества подключений к WiFi точке

String network_name = "";  // переменная для хранения названия вашей WiFi сети
String network_pass = "";  // переменная для хранения пароля вашей WiFi сети
String telegram_bot = "";  // переменная для хранения токена вашего Telegram бота
String your_chat_id = "";  // переменная для хранения указанных вами chat id
String motion_delay = "";  // переменная для хранения задержки при обнаружении движения
String flash_onoff  = "";  // переменная для хранения включения вспышки при фотографировании
String my_settings  = "-"; // переменная для хранения параметров работы вашей Telegram сигнализации

String first_chat_id  = ""; // переменная для хранения первого chat id
String second_chat_id = ""; // переменная для хранения второго chat id
String third_chat_id  = ""; // переменная для хранения третьего chat id
int firstcomma;             // переменная для хранения индекса первой разделительной запятой
int secondcomma;            // переменная для хранения индекса второй разделительной запятой

int settings_length; // переменная для хранения длинны параметров работы вашей Telegram сигнализации

int first_hash;   // переменная для хранения индекса первого разделительного слеша
int second_hash;  // переменная для хранения индекса второго разделительного слеша
int third_hash;   // переменная для хранения индекса третьего разделительного слеша
int fourth_hash;  // переменная для хранения индекса четвёртого разделительного слеша
int fifth_hash;   // переменная для хранения индекса пятого разделительного слеша
int sixth_hash;   // переменная для хранения индекса шестого разделительного слеша
int seventh_hash; // переменная для хранения индекса седьмого разделительного слеша

String HTMLContent;  // переменная для хранения Web страницы
String SPIFFSOutput; // переменная для хранения результата записи в SPIFFS память

int bot_update = 500; // переменная для хранения времени обновления Telegram бота
unsigned long last_message;   // переменная для хранения времени получения последнего сообщения

camera_fb_t * fb = NULL;            // объявляем объект для хранения изображения
httpd_handle_t stream_httpd = NULL; // объявляем объект для управления HTTP сервером

static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=123456789000000000000987654321"; // переменная для хранения контента видео потока  
static const char* _STREAM_BOUNDARY = "\r\n--123456789000000000000987654321\r\n";                              // переменная для хранения окончания формы отправки видео потока
static const char* _STREAM_PART  = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";                   // переменная для хранения формы отправки видео потока

const char* telegram_domain = "api.telegram.org"; // переменная для хранения адреса Telegram api

String main_head; // переменная для хранения формы отправки фотографии
String main_tail; // переменная для хранения окончания формы отправки фотографии
String all_data;  // переменная для хранения всех полученных данных
String all_body;  // переменная для хранения тела

uint32_t image_length; // переменная для хранения длинны фотографии
uint32_t extra_length; // переменная для хранения длинны общей формы отправки фотографии
uint32_t total_length; // переменная для хранения всей длинны

String send_photo_return;      // переменная для хранения результата отправки фотографии
bool send_photo_state;         // переменная для хранения того, отправлена ли фотография
char client_available_data;    // переменная для хранения данных полученных от Telegram api 
unsigned long send_photo_time; // переменная для хранения времени вызова отправки фотографии
unsigned long flash_on_millis; // переменная для хранения времени включения вспышки

String user_name; // переменная для хранения вашего имени в Telegram
String chat_id;   // переменная для хранения chat id Telegram пользователя
String message;   // переменная для хранения полученного сообщения
String welcome;   // переменная для хранения приветственного сообщения
String ESP32IP;   // переменная для хранения локального IP адреса 

int  chat_id_count = 1; // переменная для хранения количества указанных вами chat id
bool chat_id_matches;   // переменная для хранения того, совпадает ли chat id

bool add_chat_id     = false; // переменная для хранения того, получен ли вызов добавить ещё chat id
bool del_chat_id     = false; // переменная для хранения того, получен ли вызов удалить chat id
bool set_pir_delay   = false; // переменная для хранения того, получен ли вызов установить задержку при обнаружении движения
bool set_flash_onoff = false; // переменная для хранения того, получен ли вызов установить включение вспышки при фотографировании

bool button_state = false; // переменная для хранения состояния кнопки сброса параметров работы
bool button_flag  = false; // переменная для хранения состояния флажка кнопки сброса параметров работы
bool flash_state  = false; // переменная для хранения состояния вспышки на плате ESP32-CAM
long button_timer = 0;     // переменная для хранения времени последнего нажатия кнопки сброса параметров работы

bool send_photo_call = false; // переменная для хранения того, получен ли вызов отправить фотографию
bool jpeg_converted  = false; // переменная для хранения того, удалось ли конвертировать изображение в JPEG
bool start_stream    = false; // переменная для хранения того, получен ли вызов запустить видео поток на HTTP сервер
bool server_onoff    = false; // переменная для хранения того, запущен ли HTTP сервер с видео потоком
bool start_alarm     = false; // переменная для хранения того, запущена ли отправка фотографий при обнаружении движения
bool end_stream      = false; // переменная для хранения того, получен ли вызов остановить видео поток на HTTP сервер
bool web_onoff       = false; // переменная для хранения того, запущен ли Web сервер для настройки параметров работы
bool pir_state       = false; // переменная для хранения состояния датчика движения

unsigned long timing; // переменная для хранения millis

// функция для настройки камеры на плате ESP32-CAM
bool setupCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000; // указываем частоту камеры
  config.pixel_format = PIXFORMAT_JPEG; // указываем формат изображения (YUV422,GRAYSCALE,RGB565,JPEG)
  // Указываем разрешение фотографии в зависимости от того, поддерживает ли ваша плата PSRAM
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA; // указываем разрешение изображения (UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA)
    config.jpeg_quality = 10; // указываем качество изображения (чем меньше, тем больше качество)
    config.fb_count = 2;
  } else { // иначе
    config.frame_size = FRAMESIZE_SVGA; // указываем разрешение изображения (UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA)
    config.jpeg_quality = 12; // указываем качество изображения (чем меньше, тем больше качество)
    config.fb_count = 1;
  }

  esp_err_t err = esp_camera_init(&config); // инициализируем камеру
  if (err != ESP_OK) { // если обнаружена ошибка при запуске камеры
    Serial.printf("Возникла следующая ошибка камеры: 0x%x ", err); // выводим ошибку камеры
    return false; // возвращяем false, не удалось настроить камеру
  }

  sensor_t * s = esp_camera_sensor_get(); // объявляем объект для настройки параметров изображения
  if (s->id.PID == OV3660_PID) { // если определёна камера "OV3660"
    s->set_vflip(s, 1);       // указываем ориентацию изображения
    s->set_brightness(s, 1);  // указываем яркость изображения
    s->set_saturation(s, -2); // указываем насыщение изображения
  }
  s->set_framesize(s, FRAMESIZE_SVGA); // указываем разрешение изображения
  return true; // возвращяем true, камера успешно настроена
}

// функция для подсчёта количества подключений к WiFi точке
void WiFiPointConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  connections += 1;
  Serial.println("Количество подключений: " + String(connections));
}

WebServer webpoint(80);  // объявляем объект отвечающий за Web сервер
WiFiClientSecure client; // объявляем клиент для безопасного WiFi соединения

// функция для получения данных от Web клиента
void handleRoot() {
  HTMLContent = WebPage; // записываем Web страницу из PROGMEM
  webpoint.send(200, "text/html", HTMLContent + SPIFFSOutput); // отправляем Web страницу + результат записи в SPIFFS память

  if (webpoint.hasArg("network")) { // если Web сервер содержит аргумент названия вашей WiFi сети
    network_name = webpoint.arg("network"); // записываем название вашей WiFi сети
    Serial.print("Название WiFi сети: " + String(network_name));   
  }
  if (webpoint.hasArg("password")) { // если Web сервер содержит аргумент пароля вашей WiFi сети
    network_pass = webpoint.arg("password"); // записываем пароль вашей WiFi сети
    Serial.print("  Пароль WiFi сети: " + String(network_pass));
  }
  if (webpoint.hasArg("bot_token")) { // если Web сервер содержит аргумент токена вашего Telegram бота
    telegram_bot = webpoint.arg("bot_token"); // записываем токен вашего Telegram бота
    Serial.print("  Токен Telegram бота: " + String(telegram_bot));
  }
  if (webpoint.hasArg("chat_id")) { // если Web сервер содержит аргумент указанных вами chat id
    first_chat_id  = ""; // обнуляем переменную для хранения первого chat id
    second_chat_id = ""; // обнуляем переменную для хранения второго chat id
    third_chat_id  = ""; // обнуляем переменную для хранения третьего chat id
    
    your_chat_id = (webpoint.arg("chat_id")); // записываем указанные вами chat id
    firstcomma   = your_chat_id.indexOf(","); // записываем индекс первой разделительной запятой
    secondcomma  = your_chat_id.indexOf(",", firstcomma + 1); // записываем индекс второй разделительной запятой

    first_chat_id  = your_chat_id.substring(0, firstcomma); // записываем первый chat id
    if (firstcomma != -1)  {second_chat_id = your_chat_id.substring(firstcomma + 1, secondcomma);} // записываем второй chat id, если вы его указали
    if (secondcomma != -1) {third_chat_id  = your_chat_id.substring(secondcomma + 1, your_chat_id.length());} // записываем третий chat id, если вы его указали
    Serial.print(" Ваши chat id: " + String(your_chat_id));
  }
  if (webpoint.hasArg("pir_delay")) { // если Web сервер содержит аргумент задержки датчика движения
    motion_delay = (webpoint.arg("pir_delay")); // записываем задержку датчика движения
    Serial.print("  Задержка PIR датчика: " + String(motion_delay));
  }
  if (webpoint.hasArg("flashon")) { // если Web сервер содержит аргумент отключения вспышки при фотографировании
    flash_onoff = "1"; // записываем включение вспышки при фотографировании
    Serial.println("  Включение вспышки при фотографировании!");
  }
  if (webpoint.hasArg("flashoff")) { // если Web сервер содержит аргумент включения вспышки при фотографировании
    flash_onoff = "0"; // записываем отключение вспышки при съёмке фотографии
    Serial.println("  Отключение вспышки при фотографировании!");
  }

  if (network_name != "" and network_pass != "" and telegram_bot != "" and your_chat_id != "" and motion_delay != "") { // если все параметры работы Telegram сигнализации указаны
    // соеденяем и записываем параметры работы вашей Telegram сигнализации
    if (first_chat_id != "") { // если вы указали один chat id, то соеденяем параметры работы
      chat_id_count = 1; // записываем количество указанных вами chat id
      my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
    }
    if (second_chat_id != "") { // если вы указали два chat id, то соеденяем параметры работы
      chat_id_count = 2; // записываем количество указанных вами chat id
      my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
    }
    if (third_chat_id != "") { // если вы указали три chat id, то соеденяем параметры работы
      chat_id_count = 3; // записываем количество указанных вами chat id
      my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + second_chat_id + "#" + third_chat_id + "#" + motion_delay + "#" + flash_onoff;
    }

    File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с настройками для записи
    Serial.println("Ваши параметры работы: " + String(my_settings));
    if (!settings) { // если файл не удалось открыть
      SPIFFSOutput = SPIFFSError; // записываем текст при ошибке записи в SPIFFS память
      return; // выходим из функции
      Serial.println("Не удалось открыть файл с настройками для записи :(");
    } else {
      Serial.println("Файл с настройками успешно открыт для записи!");
    }

    if (settings.print(my_settings)) { // записываем параметры работы вашей Telegram сигнализации в файл с настройками
      SPIFFSOutput = SPIFFSWrite; // записываем страницу при успешной записи в SPIFFS память
      Serial.println("Файл был успешно записан!");
    } else {
      SPIFFSOutput = SPIFFSError; // записываем страницу при ошибке записи в SPIFFS память
      Serial.println("Не удалось записать файл :(");
    }
   settings.close(); // закрываем файл с настройками
  }
}

// функция для отправки фотографий вам на Telegram
void sendPhoto() {
  send_photo_return = "";   // обнуляем результат отправки фотографии
  fb = esp_camera_fb_get(); // получаем фотографию   
  if (!fb) { // если не удалось сделать фотографию
    send_photo_return = "Не удалось сделать фотографию"; // записываем то, что не удалось сделать фотографию
    Serial.println("Не удалось сделать фотографию :(");
  } else {
    Serial.println("Фотография получена!");
  }

  if (send_photo_return != "Не удалось сделать фотографию") { // если фотография получена
    Serial.println("Подключаемся к " + String(telegram_domain)); 
    if (client.connect(telegram_domain, 443)) { // если клиент подключен к Telegram api
      Serial.println("Подключение прошло успешно!");
      // записываем форму отправки фотографии
      main_head  = "--WorldOfArduino\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chat_id + "\r\n--WorldOfArduino\r";
      main_head += "\nContent-Disposition: form-data; name=\"photo\";filename=\"esp32-photo.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
      // записываем окончание формы отправки фотографии
      main_tail  = "\r\n--WorldOfArduino--\r\n";

      image_length = fb->len;                                 // записываем длинну фотографии
      extra_length = main_head.length() + main_tail.length(); // записываем длинну общей формы отправки фотографии
      total_length = image_length + extra_length;             // записываем общую длинну

      // отправляем форму отправки фотографии на Telegram api
      client.println("POST /bot"+telegram_bot+"/sendPhoto HTTP/1.1"); // отправляем токен вашего Telegram бота
      client.println("Host: " + String(telegram_domain));             // отправляем адрес Telegram api
      client.println("Content-Length: " + String(total_length));      // отправляем общую длинну
      client.println("Content-Type: multipart/form-data; boundary=WorldOfArduino"); // отправляем окончание формы отпраки фотографии
      client.println();
      client.print(main_head); // отправляем форму отправки фотографии

      uint8_t *fbBuf = fb->buf; // записываем фотографию из буфера
      size_t fbLen  = fb->len;  // записываем длинну фотографии из буфера
      // отправляем фотографию из буфера на Telegram api
      for (size_t n = 0; n < fbLen; n = n + 1024) {
        if (n + 1024 < fbLen) {
          client.write(fbBuf, 1024);
          fbBuf += 1024;
        } else if (fbLen%1024 > 0) {
          size_t remainder = fbLen%1024;
          client.write(fbBuf, remainder);
        }
      }

      client.print(main_tail);  // отправляем окончание формы отправки фотографии 
      esp_camera_fb_return(fb); // возвращяем фотографию

      send_photo_time = millis(); // записываем время отправки фотографии
      send_photo_state = false;   // записываем то, что фотография не отправлена

      while ((send_photo_time + send_photo_wait) > millis()) { // пока время ожидания отправки фотографии не исчерпано
        Serial.print("."); delay(250);
        while (client.available()) { // если данные от Telegram api полученны
          client_available_data = client.read(); // записываем данные полученные от Telegram api
          if (client_available_data == '\n') { // если полученные данные равны "\n"
            if (all_data.length() == 0) send_photo_state = true; // если длинна всех данных равна нулю, то фотография отправлена
              all_data = ""; // очищаем переменную для хранения всех полученных данных
            } 
            else if (client_available_data != '\r') // если полученные данные не равны "\r"
              all_data += String(client_available_data); // добавляем к всем данным, данные полученные от Telegram api
            if (send_photo_state == true) all_body += String(client_available_data); // если фотография отправлена, то добавляем к телу, данные полученные от Telegram api
          send_photo_time = millis(); // записываем время отправки фотографии
        }
        if (all_body.length() > 0) break; // если длинна тела больше нуля, то выходим из цикла 
      }
      client.stop(); // прекращаем соединение с Telegram api
      Serial.println(all_body);
      if (all_body.indexOf("false") != -1) { // если не удалось отправить фотографию
        send_photo_return = "Не удалось отправить фотографию"; // записываем то, что не удалось отправить фотографию
        Serial.println("Не удалось отправить фотографию :(");
      }
    } else {
      send_photo_return = "Не удалось подключиться к api.telegram.org"; // записываем то, что не удалось подключиться к Telegram api
      Serial.println("Не удалось подключиться к api.telegram.org :(");
    }
  }
}

// функция для отправки видео потока на HTTP сервер
static esp_err_t stream_handler(httpd_req_t *req){
  esp_err_t res = ESP_OK;    // переменная для хранения результата отправки HTTP потока
  size_t _jpg_buf_len = 0;   // переменная для хранения длинны JPEG буфера
  uint8_t * _jpg_buf = NULL; // переменная для хранения JPEG буфера
  char * part_buf[64];       // переменная для хранения JPEG буфера в char массиве

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE); // получаем  результат установки типа HTTP ответа
  if (res != ESP_OK) { // если не удалось установить тип HTTP ответа
    return res; // выходим из функции
  }

  while (true) { // бесконечный цикл
    fb = esp_camera_fb_get(); // получаем изображение
    if (!fb) { // если не удалось получить изображение
      res = ESP_FAIL; // записываем ошибку ESP32-CAM
      Serial.println("Не удалось сделать фотографию :(");
    } else {
      if (fb->width >= 400) { // если разрешение изображения больше или равно 400 пикселей
        if (fb->format != PIXFORMAT_JPEG) { // если формат изображения не JPEG
          jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len); // конвертируем изображение в формат JPEG
          esp_camera_fb_return(fb); // возвращяем изображение
          fb = NULL; // очищаем буфер для изображения
          if (!jpeg_converted) { // если не удалось конвертировать изображение
            res = ESP_FAIL; // записываем ошибку ESP32-CAM
            Serial.println("Не удалось конвертировать JPEG :(");
          }
        } else {
          _jpg_buf_len = fb->len; // записываем длинну JPEG буфера
          _jpg_buf = fb->buf;     // записываем JPEG буфер
        }
      }
    }
    if (res == ESP_OK) { // если во время работы ESP32-CAM не возникла ошибка
      size_t hlen = snprintf((char *)part_buf, 64, _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen); // отправляем JPEG буфер в char
    }
    if (res == ESP_OK) { // если во время работы ESP32-CAM не возникла ошибка
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len); // отправляем JPEG буфер и его длинну
    }
    if (res == ESP_OK) { // если во время работы ESP32-CAM не возникла ошибка
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY)); // отправляем окончание формы видео
    }
    if (fb) { // если изображение получено
      esp_camera_fb_return(fb); // возвращяем изображение
      fb = NULL;       // очищаем буфер для изображения
      _jpg_buf = NULL; // очищаем JPEG буфер
    } else if (_jpg_buf) { // иначе, если JPEG содержит данные
      free(_jpg_buf);  // освобождаем память под JPEG буфер
      _jpg_buf = NULL; // очищаем JPEG буфер
    }
    if(res != ESP_OK){ // если возникла ошибка при работе ESP32-CAM
      break; // выходим из цикла
    }
  }
  return res; // возвращяем результат работы ESP32-CAM или отправки HTTP потока
}

// функция для проверки того, является ли полученное сообщение числом
bool isNumber(String message_chat_id) {
  for (int i = 0; i < message_chat_id.length(); i ++) { // проверяем каждый символ строки
    if (isDigit(message_chat_id.charAt(i))) { // если символ строки является цифрой
      continue; // пропускаем итерацию цикла проверки строки
    } else { // иначе
      return false; // возвращаем false, строка не является числом
    }
  }
  return true; // возвращаем true, строка является числом
}

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // отключаем детектор отключения питания
  Serial.begin(115200); // настраиваем скорость работы COM порта

  pinMode(flash_led_pin, OUTPUT);           // настраиваем пин к которому подключена вспышка как выход
  pinMode(reset_button_pin, INPUT_PULLUP);  // настраиваем пин к которому подключена кнопка сброса параметров работы вашей Telegram сигнализации как вход
  pinMode(motion_sensor_pin, INPUT_PULLUP); // настраиваем пин к которому подключен датчик движения как вход
  digitalWrite(flash_led_pin, LOW);         // отключаем вспышку

  if (setupCamera()) { // если не удалось настроить камеру на плате ESP32-CAM
    Serial.println("Камера успешно настроена!");
  } else {
    return; // выходим из функции
    ESP.restart(); // перезапускаем ESP32-CAM
    Serial.println("Не удалось настроить камеру :(");
  }

  if (SPIFFS.begin(true)){ // если не удалось монтировать SPIFFS память
    Serial.println("Монтирование SPIFFS прошло успешно!");
  } else {
    return; // выходим из функции
    Serial.println("Ошибка при монтировании SPIFFS :(");  
  }

  File settings = SPIFFS.open("/Settings.txt"); // открываем файл с параметрами работы для чтения
  if (settings) { // если не удалось открыть файл с параметрами работы
    Serial.println("Файл с настройками успешно открыт для чтения!");
  } else {
    return; // выходим из функции
    Serial.println("Не удалось открыть файл с настройками для чтения :(");
  }

  while (settings.available()) { // прочитываем данные из файла с параметрами работы
    my_settings = settings.readString(); // записываем данные из файла с параметрами работы
    settings_length = my_settings.length(); // записываем длину параметров работы
    Serial.println("Содержимое файла: " + String(my_settings));

    first_hash   = my_settings.indexOf('#');                  // записываем индекс первого разделительного слеша
    second_hash  = my_settings.indexOf('#', first_hash + 1);  // записываем индекс второго разделительного слеша
    third_hash   = my_settings.indexOf('#', second_hash + 1); // записываем индекс третьего разделительного слеша
    fourth_hash  = my_settings.indexOf('#', third_hash + 1);  // записываем индекс четвертого разделительного слеша
    fifth_hash   = my_settings.indexOf('#', fourth_hash + 1); // записываем индекс пятого разделительного слеша
    sixth_hash   = my_settings.indexOf('#', fifth_hash + 1);  // записываем индекс шестого разделительного слеша
    seventh_hash = my_settings.indexOf('#', sixth_hash + 1);  // записываем индекс седьмого разделительного слеша

    network_name   = my_settings.substring(0, first_hash);               // записываем название вашей WiFi сети
    network_pass   = my_settings.substring(first_hash + 1, second_hash); // записываем пароль вашей WiFi сети
    telegram_bot   = my_settings.substring(second_hash + 1, third_hash); // записываем токен вашего Telegram бота
    if (sixth_hash == -1) { // если вы указали один chat id
      chat_id_count = 1;                                                      // записываем количество указанных вами chat id
      first_chat_id = my_settings.substring(third_hash + 1, fourth_hash);     // записываем первый chat id
      motion_delay  = my_settings.substring(fourth_hash + 1, fifth_hash);     // записываем задержку при обнаружении движения
      flash_onoff   = my_settings.substring(fifth_hash + 1, settings_length); // записываем включение вспышки при фотографировании
    }
    if (seventh_hash == -1) { // если вы указали два chat id
      chat_id_count  = 2;                                                      // записываем количество указанных вами chat id
      first_chat_id  = my_settings.substring(third_hash + 1, fourth_hash);     // записываем первый chat id
      second_chat_id = my_settings.substring(fourth_hash + 1, fifth_hash);     // записываем второй chat id
      motion_delay   = my_settings.substring(fifth_hash + 1, sixth_hash);      // записываем задержку при обнаружении движения
      flash_onoff    = my_settings.substring(sixth_hash + 1, settings_length); // записываем включение вспышки при фотографировании
    }
    if (seventh_hash != -1 and sixth_hash != -1) { // если вы указали три chat id
      chat_id_count = 3;                                                         // записываем количество указанных вами chat id
      first_chat_id  = my_settings.substring(third_hash + 1, fourth_hash);       // записываем первый chat id
      second_chat_id = my_settings.substring(fourth_hash + 1, fifth_hash);       // записываем второй chat id
      third_chat_id  = my_settings.substring(fifth_hash + 1, sixth_hash);        // записываем третий chat id
      motion_delay   = my_settings.substring(sixth_hash + 1, seventh_hash);      // записываем задержку при обнаружении движения
      flash_onoff    = my_settings.substring(seventh_hash + 1, settings_length); // записываем включение вспышки при фотографировании
    }

    // выводим параметры работы вашей Telegram сигнализации
    Serial.println("Название WiFi сети: "   + String(network_name));
    Serial.println("Пароль WiFi сети: "     + String(network_pass));
    Serial.println("Токен Telegram бота: "  + String(telegram_bot));
    Serial.println("Первый chat id: "       + String(first_chat_id));
    Serial.println("Второй chat id: "       + String(second_chat_id));
    Serial.println("Третий chat id: "       + String(third_chat_id));
    Serial.println("Задержка PIR датчика: " + String(motion_delay));
    Serial.println("Включение вспышки: "    + String(flash_onoff));
  }

  settings.close(); // закрываем файл с параметрами работы

  if (my_settings == "-") { // если ваши параметры работы не указаны
    Serial.println("Режим Web сервера запущен!");
    WiFi.softAP(ssid_name, password); // запускаем WiFi точку
    WiFi.onEvent(WiFiPointConnected, SYSTEM_EVENT_AP_STACONNECTED); // указываем функцию, выполняющуюся при подключении клиента к WiFi точке
    IPAddress MyIP = WiFi.softAPIP(); // указываем IP адрес платы ESP32-CAM

    webpoint.on("/", handleRoot); // указываем функцию получения данных от WiFi клиента
    webpoint.begin(); // запускаем Web сервер на плате ESP32-CAM
    web_onoff = true; // записываем то, что Web сервер запущен

    Serial.println("Web точка запущена!");
    Serial.println("Пароль от ESP32-CAM: " + String(password));
    Serial.print("Адрес Web сервера: "); Serial.println(MyIP);
  }
}

void loop() {
  if (web_onoff == true) { // если Web сервер запущен
    webpoint.handleClient(); // получаем и отправляем данные WiFi клиенту
  }
  if (my_settings != "-") { // если ваши параметры работы указаны
    if (WiFi.status() != WL_CONNECTED and button_state == false) { // если плата ESP32-CAM не подключена к WiFi сети и кнопка сброса не нажата
      WiFi.begin((const char*)network_name.c_str(), (const char*)network_pass.c_str()); // подключаемся к вашей WiFi сети
      Serial.println("Подключаемся к " + String(network_name));

      if (WiFi.waitForConnectResult() == WL_CONNECTED) { // если плата ESP32-CAM не подключилась к WiFi сети
        Serial.println("Подключились к WiFi сети!");
        IPAddress LocalIP = WiFi.localIP(); // записываем локальный IP адрес
        for (int i = 0; i < 4; i ++) { // конвертируем локальный IP адрес в строку
          ESP32IP += i ? "." + String(LocalIP[i]) : String(LocalIP[i]);
        }
        Serial.println("Локальный IP адрес: " + String(ESP32IP));
      }
    }

    button_state = !digitalRead(reset_button_pin); // прочитываем состояние кнопки сброса параметров работы
    pir_state = digitalRead(motion_sensor_pin);    // прочитываем состояние датчика движения

    if (button_state && !button_flag && millis() - button_timer > 150) { // если кнопка сброса параметров работы нажата
      button_timer = millis(); // записываем время нажатия кнопки
      button_flag = true; // поднимаем флажок
      Serial.println("Кнопка сброса настроек нажата!");
    }

    if (!button_state && button_flag && millis() - button_timer > 500) { // если кнопка сброса параметров работы отпущена
      button_timer = millis(); // записываем время нажатия кнопки
      button_flag = false; // опускаем флажок
      Serial.println("Кнопка сброса настроек отпущена!");
    }
  
    if (button_state && button_flag && millis() - button_timer > button_hold_time) { // если кнопка сброса параметров работы удерживается более 3000 миллисекунд
      // очищаем файл с настройками    
      button_timer = millis(); // записываем время нажатия кнопки
      Serial.println("Кнопка сброса настроек зажата!");
      digitalWrite(flash_led_pin, HIGH); // включаем вспышку
      File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с настройками для записи

      if (!settings) { // если не удалось открыть файл с параметрами работы
        return; // выходим из функции
        Serial.println("Не удалось открыть файл с настройками для очистки :(");
      } else {
        Serial.println("Файл с настройками успешно открыт для очистки!");
      }
    
      if (settings.print("-")) { // очищаем файл с параметрами работы
        Serial.println("Файл был успешно очищен!");;
      } else {
        Serial.println("Не удалось очистить файл :(");
      }

      SPIFFSOutput = "";                // очищаем переменную для хранения результата записи в SPIFFS
      delay(500);                       // делаем задержку для выключения вспышки
      digitalWrite(flash_led_pin, LOW); // отключаем вспышку
      settings.close();                 // закрываем файл с настройками
      ESP.restart();                    // перезапускаем ESP32-CAM
    }

    if (WiFi.status() == WL_CONNECTED) { // если плата ESP32-CAM подключена к WiFi сети
      UniversalTelegramBot bot(telegram_bot, client); // объявляем Telegram бота и указываем его токен и WiFi клиент
      bot.longPoll = 1; // указываем как долго бот будет ждать новых сообщений, прежде чем возвращать функцию "получено сообщения" в секундах
      client.setInsecure();

      if (millis() > last_message + bot_update) { // делаем задержку при проверке нового сообщения
        int numNewMessages = bot.getUpdates(bot.last_message_received + 1); // прочитываем новые сообщения полученные от Telegram пользователя
        while (numNewMessages) { // если получено новое сообщение от Telegram пользователя
          for (int i = 0; i < numNewMessages; i ++) {
            Serial.println("Получен ответ от Telegram пользователя!");
            chat_id = String(bot.messages[i].chat_id); // записываем chat id Telegram пользователя
            message = bot.messages[i].text;            // записываем сообщение полученное от Telegram пользователя
            user_name = bot.messages[i].from_name;     // записываем ваше имя в Telegram
            if (user_name == "") user_name = "Гость";  // если не удалось прочитать ваше имя в Telegram
   
            if (chat_id_count == 1) { // если вы указали один chat id
              if (first_chat_id == chat_id) { // если chat id Telegram пользователя совпадает с первым chat id
                chat_id_matches = true; // записываем то, что chat id совпадает
              } else {
                chat_id_matches = false; // записываем то, что chat id не совпадает
              }
            }
            if (chat_id_count == 2) { // если вы указали два chat id
              if (first_chat_id == chat_id or second_chat_id == chat_id) { // если chat id Telegram пользователя совпадает с первым или вторым chat id
                chat_id_matches = true; // записываем то, что chat id совпадает
              } else {
                chat_id_matches = false; // записываем то, что chat id не совпадает
              }
            }
            if (chat_id_count == 3) { // если вы указали три chat id
              if (first_chat_id == chat_id or second_chat_id == chat_id or third_chat_id == chat_id) { // если chat id Telegram пользователя совпадает с первым, вторым или третьим chat id
                chat_id_matches = true; // записываем то, что chat id совпадает
              } else {
                chat_id_matches = false; // записываем то, что chat id не совпадает
              }
            }

            if (chat_id_matches == true) { // если chat id Telegram пользователя совпадает с одним из указанных вами chat id
              web_onoff = false; // записываем то, что Web сервер остановлен
              if (add_chat_id == false and del_chat_id == false and set_pir_delay == false and set_flash_onoff == false) { // если нет вызова настроить параметры работы
                if (message == "/start") { // если получено сообщение "/start"
                  // записываем и отправляем приветственное сообщение
                  welcome =  "Здравствуйте, " + user_name + "! Вас приветствует ESP32-CAM сигнализация\n";
                  welcome += "/takephoto - отправить фотографию\n";
                  welcome += "/sendsettings - отправить настройки\n";
                  welcome += "/startalarm - включить сигнализацию\n";
                  welcome += "/stopalarm - отключить сигнализацию\n";
                  welcome += "/startstream - запустить трансляцию\n";
                  welcome += "/stopstream - остановить трансляцию\n";
                  welcome += "/turnonflash - включить вспышку\n";
                  welcome += "/turnoffflash - отключить вспышку\n";
                  welcome += "/addchatid - добавить ещё chat id\n";
                  welcome += "/delchatid - удалить chat id\n";
                  welcome += "/pirdelay - установить PIR задержку\n";
                  welcome += "/flashonoff - включать ли вспышку\n";
                  bot.sendMessage(chat_id, welcome, "Markdown");  
                  Serial.println(welcome);
                }

                if (message == "/takephoto") { // если получено сообщение "/takephoto"
                  send_photo_call = true; // записываем вызов отправить фотографию
                }

                if (message == "/sendsettings") { // если получено сообщение "/sendsettings"
                  // записываем и отправляем параметры работы вашей Telegram сигнализации
                  welcome =  "Настройки ESP32-CAM сигнализации:\n";
                  welcome += "Название WiFi сети: "   + network_name + "\n";
                  welcome += "Пароль WiFi сети: "     + network_pass + "\n";
                  welcome += "Токен Telegram бота: "  + telegram_bot + "\n";
                  if (chat_id_count == 1) { // если вы указали один chat id
                    welcome += "Первый chat id: " + first_chat_id + "\n";
                  }
                  if (chat_id_count == 2) { // если вы указали два chat id
                    welcome += "Первый chat id: " + first_chat_id  + "\n";
                    welcome += "Второй chat id: " + second_chat_id + "\n";
                  }
                  if (chat_id_count == 3) { // если вы указали три chat id
                    welcome += "Первый chat id: " + first_chat_id  + "\n";
                    welcome += "Второй chat id: " + second_chat_id + "\n";
                    welcome += "Третий chat id: " + third_chat_id  + "\n";
                  }
                  welcome += "Задержка PIR датчика: " + motion_delay + "\n";
                  if (flash_onoff == "1") { // если включение вспышки при фотографировании установлено
                    welcome += "Включение вспышки: да\n";
                  } else {
                    welcome += "Включение вспышки: нет\n";
                  }
                  bot.sendMessage(chat_id, welcome, "Markdown");
                  Serial.println(welcome);
                }

                if (message == "/turnonflash") { // если получено сообщение "/turnonflash"
                  if (flash_state == false) { // если вспышка отключена
                    flash_state = true; // записываем то, что вспышка включена
                    flash_on_millis = millis(); // записываем время включения вспышки
                    digitalWrite(flash_led_pin, HIGH); // включаем вспышку
                    bot.sendMessage(chat_id, "Вспышка включена!", "Markdown");
                    Serial.println("Вспышка включена!"); 
                  } else {
                    bot.sendMessage(chat_id, "Вспышка уже включена!", "Markdown");
                    Serial.println("Вспышка уже включена!");
                  }
                }

                if (message == "/turnoffflash") { // если получено сообщение "/turnoffflash"
                  if (flash_state == true) { // если был вызов включить вспышку
                    flash_state = false; // записываем то, что вспышка отключена
                    digitalWrite(flash_led_pin, LOW); // отключаем вспышку
                    bot.sendMessage(chat_id, "Вспышка отключена!", "Markdown");
                    Serial.println("Вспышка отключена!");
                  } else {
                    bot.sendMessage(chat_id, "Вы не включали вспышку!", "Markdown");
                    Serial.println("Вы не включали вспышку!");
                  }
                }

                if (message == "/startstream") { // если получено сообщение "/startstream"
                  if (server_onoff == false) { // если HTTP сервер остановлен
                    start_stream = true; // записываем вызов включить видео поток на HTTP сервер
                    WiFi.softAPdisconnect(true); // отключаем WiFi точку
                  } else {
                    bot.sendMessage(chat_id, "Видео поток уже запущен!", "Markdown");
                    Serial.println("Видео поток уже запущен!");
                  }
                }

                if (message == "/stopstream") { // если получено сообщение "/stopstream"
                  if (server_onoff == true) { // если HTTP сервер запущен
                    end_stream = true; // записываем вызов отключить видео поток на HTTP сервер 
                  } else {
                    bot.sendMessage(chat_id, "Видео поток уже остановлен!", "Markdown");
                    Serial.println("Видео поток уже остановлен!");
                  }
                }

                if (message == "/startalarm") { // если получено сообщение "/startalarm"
                  start_alarm = true; // записываем вызов включить отправку фотографий при обнаружении движения
                  bot.sendMessage(chat_id, "Ваша сигнализация запущена!", "Markdown");
                  Serial.println("Ваша сигнализация запущена!");
                }

                if (message == "/stopalarm") { // если получено сообщение "/stopalarm"
                  start_alarm = false; // отключаем отправку фотографий при обнаружении движения
                  bot.sendMessage(chat_id, "Ваша сигнализация отключена!", "Markdown");
                  Serial.println("Ваша сигнализация отключена!");
                }

                if (message == "/addchatid") { // если получено сообщение "/addchatid"
                  if (chat_id_count != 3) {    // если вы указали меньше трёх chat id
                    message = "-";             // очищаем переменную с сообщением
                    add_chat_id = true;        // записываем вызов добавить ещё chat id
                    bot.sendMessage(chat_id, "Отправьте chat id для добавления", "Markdown");
                    Serial.println("Отправьте chat id для добавления");
                  } else {
                    bot.sendMessage(chat_id, "Количество ваших chat id исчерпано!", "Markdown");
                    Serial.println("Количество ваших chat id исчерпано!");
                  }
                }

                if (message == "/delchatid") { // если получено сообщение "/delchatid"
                  if (chat_id_count != 1) {    // если вы указали два chat id
                    message = "-";             // очищаем переменную с сообщением
                    del_chat_id = true;        // записываем вызов удалить chat id
                    bot.sendMessage(chat_id, "Отправьте chat id для удаления", "Markdown");
                    Serial.println("Отправьте chat id для удаления");
                  } else {
                    bot.sendMessage(chat_id, "Вы не можете удалить последний chat id!", "Markdown");
                    Serial.println("Вы не можете удалить последний chat id!");
                  }
                }

                if (message == "/pirdelay") { // если получено сообщение "/pirdelay"
                  message = "-";              // очищаем переменную с сообщением
                  set_pir_delay = true;       // записываем вызов установить задержку при обнаружении движения
                  bot.sendMessage(chat_id, "Отправьте задержку PIR датчика", "Markdown");
                  Serial.println("Отправьте ещё один chat id");
                }

                if (message == "/flashonoff") { // если получено сообщение "/flashonoff"
                  message = "-";                // очищаем переменную с сообщением
                  set_flash_onoff = true;       // записываем вызов установить включение вспышки при фотографировании
                  bot.sendMessage(chat_id, "Установите включение вспышки:\nДа - включать вспышку\nНет - выключать вспышку", "Markdown");
                  Serial.println("Отправьте ещё один chat id");
                }
              } else { // если есть вызов добавить или удалить chat id
                if (message != "-") { // если был получен chat id
                  if (add_chat_id == true) { // если получен вызов добавить chat id
                    if (isNumber(message)) { // если отправленный вами chat id является числом
                      if (chat_id_count == 1) { // если вы указали один chat id
                        second_chat_id = message; // записываем второй chat id
                        // переписываем ваши параметры работы
                        my_settings = network_name + "#" + network_pass + "#" + telegram_bot + "#";
                        my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                      }
                      if (chat_id_count == 2) { // если вы указали два chat id
                        third_chat_id = message; // записываем третий chat id
                        // переписываем ваши параметры работы
                        my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                        my_settings += first_chat_id + "#" + second_chat_id + "#" + third_chat_id + "#" + motion_delay + "#" + flash_onoff;
                      }
                      if (third_chat_id == "") { // если третий chat id не был записан
                        chat_id_count = 2; // записываем то, что вы указали два chat id
                      } else {
                        chat_id_count = 3; // записываем то, что вы указали три chat id
                      }

                      Serial.println("Ваши параметры работы: " + String(my_settings));
                      File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с настройками для записи
                      if (!settings) { // если файл не удалось открыть
                        add_chat_id = false; // отключаем вызов добавить chat id
                        bot.sendMessage(chat_id, "Не удалось открыть файл с настройками для записи :(", "Markdown");
                        Serial.println("Не удалось открыть файл с настройками для записи :(");
                      } else {
                        Serial.println("Файл с настройками успешно открыт для записи!");
                      }
                      if (settings.print(my_settings)) { // записываем параметры работы вашей Telegram сигнализации в файл с параметрами работы
                        add_chat_id = false; // отключаем вызов добавить chat id
                        bot.sendMessage(chat_id, "Ещё один chat id был добавлен!", "Markdown");
                        Serial.println("Файл был успешно записан!");
                      } else {
                        add_chat_id = false; // отключаем вызов добавить chat id
                        bot.sendMessage(chat_id, "Не удалось записать файл :(", "Markdown");
                        Serial.println("Не удалось записать файл :(");
                      }
                      settings.close(); // закрываем файл с настройками
                    } else {
                      bot.sendMessage(chat_id, "Chat id должен состоять только из цифр!", "Markdown");
                      Serial.println("Chat id должен состоять только из цифр!");
                    }
                  }

                  if (del_chat_id == true) { // если получен вызов удалить chat id
                    if (isNumber(message)) { // если отправленный вами chat id является числом
                      if (message == first_chat_id or message == second_chat_id or message == third_chat_id) { // если отправленный вами chat id указан
                        if (chat_id_count == 2) { // если вы указали два chat id
                          chat_id_count = 1; // записываем то, что вы указали один chat id
                          if (message == first_chat_id) { // если отправленный вами chat id равен первому chat id
                            first_chat_id = second_chat_id; // записываем в первый chat id, значение второго chat id
                            // переписываем ваши параметры работы
                            my_settings = network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
                          }
                          if (message == second_chat_id) { // если отправленный вами chat id равен второму chat id
                            // переписываем ваши параметры работы
                            my_settings = network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
                          }
                        }
                        if (chat_id_count == 3) { // если вы указали три chat id
                          chat_id_count = 2; // записываем то, что вы указали два chat id
                          if (message == first_chat_id) { // если отправленный вами chat id равен первому chat id
                            first_chat_id = second_chat_id; // записываем в первый chat id, значение второго chat id
                            // переписываем ваши параметры работы
                            my_settings = network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
                          }
                          if (message == second_chat_id) { // если отправленный вами chat id равен второму chat id
                            second_chat_id = third_chat_id; // записываем во второй chat id, значение третьего chat id
                            // переписываем ваши параметры работы
                            my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                            my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                          }
                          if (message == third_chat_id) { // если отправленный вами chat id равен третьему chat id
                            // переписываем ваши параметры работы
                            my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                            my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                          }
                        }

                        Serial.println("Ваши параметры работы: " + String(my_settings));
                        File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с параметрами работы для записи
                        if (!settings) { // если файл не удалось открыть
                          del_chat_id = false; // отключаем вызов удалить chat id
                          bot.sendMessage(chat_id, "Не удалось открыть файл с настройками для записи :(", "Markdown");
                          Serial.println("Не удалось открыть файл с настройками для записи :(");
                        } else {
                          Serial.println("Файл с настройками успешно открыт для записи!");
                        }
                        if (settings.print(my_settings)) { // записываем параметры работы вашей Telegram сигнализации в файл с параметрами работы
                          del_chat_id = false; // отключаем вызов удалить chat id
                          bot.sendMessage(chat_id, "Отправленный chat id был удалён!", "Markdown");
                          Serial.println("Файл был успешно записан!");
                        } else {
                          del_chat_id = false; // отключаем вызов удалить chat id
                          bot.sendMessage(chat_id, "Не удалось записать файл :(", "Markdown");
                          Serial.println("Не удалось записать файл :(");
                        }
                        settings.close(); // закрываем файл с настройками
                      } else {
                        del_chat_id = false; // отключаем вызов удалить chat id
                        bot.sendMessage(chat_id, "Отправленный вами chat id не совпадает!", "Markdown");
                        Serial.println("Отправленный вами chat id не совпадает!");
                      }
                    } else {
                      bot.sendMessage(chat_id, "Chat id должен состоять только из цифр!", "Markdown");
                      Serial.println("Chat id должен состоять только из цифр!");
                    }
                  }

                  if (set_pir_delay == true) { // если получен вызов установить задержку при обнаружении движения
                    if (isNumber(message)) { // если полученное сообщение является числом
                      if (message.toInt() >= 3000) { // если полученно число меньше или равно 3000 
                        motion_delay = message; // записываем полученно сообщение
                        if (chat_id_count == 1) { // если вы указали один chat id
                          // переписываем ваши параметры работы
                          my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
                        }
                        if (chat_id_count == 2) { // если вы указали два chat id
                          // переписываем ваши параметры работы
                          my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                          my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                        }
                        if (chat_id_count == 3) { // если вы указали три chat id
                          // переписываем ваши параметры работы
                          my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                          my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                        }

                        Serial.println("Ваши параметры работы: " + String(my_settings));
                        File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с параметрами работы для записи
                        if (!settings) { // если файл не удалось открыть
                          set_pir_delay = false; // отключаем вызов установить задержку при обнаружении движения
                          bot.sendMessage(chat_id, "Не удалось открыть файл с настройками для записи :(", "Markdown");
                          Serial.println("Не удалось открыть файл с настройками для записи :(");
                        } else {
                          Serial.println("Файл с настройками успешно открыт для записи!");
                        }
                        if (settings.print(my_settings)) { // записываем параметры работы вашей Telegram сигнализации в файл с параметрами работы
                          set_pir_delay = false; // отключаем вызов установить задержку при обнаружении движения
                          bot.sendMessage(chat_id, "Задержка PIR датчика была установлена!", "Markdown");
                          Serial.println("Файл был успешно записан!");
                        } else {
                          set_pir_delay = false; // отключаем вызов установить задержку при обнаружении движения
                          bot.sendMessage(chat_id, "Не удалось записать файл :(", "Markdown");
                          Serial.println("Не удалось записать файл :(");
                        }
                      } else {
                        bot.sendMessage(chat_id, "Задержка PIR датчика должна быть не менее 3000 миллисекунд!", "Markdown");
                        Serial.println("Задержка PIR датчика должна быть не менее 3000 миллисекунд!");
                      }
                    } else {
                      bot.sendMessage(chat_id, "Задержка PIR датчика должна состоять только из цифр!", "Markdown");
                      Serial.println("Задержка PIR датчика должна состоять только из цифр!");
                    }
                  }

                  if (set_flash_onoff == true) { // если получен вызов установить включение вспышки
                    if (message == "Да" or message == "Нет") { // если полученное сообщение равно "Да" или "Нет"
                      if (message == "Да") { // если полученное сообщение равно "Да"
                        flash_onoff = "1"; // записываем включение вспышки при фотографировании
                      } else {
                        flash_onoff = "0"; // записываем включение вспышки при фотографировании
                      }
                      if (chat_id_count == 1) { // если вы указали один chat id
                        // переписываем ваши параметры работы
                        my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#" + first_chat_id + "#" + motion_delay + "#" + flash_onoff;
                      }
                      if (chat_id_count == 2) { // если вы указали два chat id
                        // переписываем ваши параметры работы
                        my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                        my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                      }
                      if (chat_id_count == 3) { // если вы указали три chat id
                        // переписываем ваши параметры работы
                        my_settings =  network_name + "#" + network_pass + "#" + telegram_bot + "#";
                        my_settings += first_chat_id + "#" + second_chat_id + "#" + motion_delay + "#" + flash_onoff;
                      }

                      Serial.println("Ваши параметры работы: " + String(my_settings));
                      File settings = SPIFFS.open("/Settings.txt", FILE_WRITE); // открываем файл с параметрами работы для записи
                      if (!settings) { // если файл не удалось открыть
                        set_flash_onoff = false; // отключаем вызов установить включение вспышки при фотографировании
                        bot.sendMessage(chat_id, "Не удалось открыть файл с настройками для записи :(", "Markdown");
                        Serial.println("Не удалось открыть файл с настройками для записи :(");
                      } else {
                        Serial.println("Файл с настройками успешно открыт для записи!");
                      }
                      if (settings.print(my_settings)) { // записываем параметры работы вашей Telegram сигнализации в файл с параметрами работы
                        set_flash_onoff = false; // отключаем вызов установить включение вспышки при фотографировании
                        bot.sendMessage(chat_id, "Включение вспышки было установлено!", "Markdown");
                        Serial.println("Файл был успешно записан!");
                      } else {
                        set_flash_onoff = false; // отключаем вызов установить включение вспышки при фотографировании
                        bot.sendMessage(chat_id, "Не удалось записать файл :(", "Markdown");
                        Serial.println("Не удалось записать файл :(");
                      }
                    } else {
                      bot.sendMessage(chat_id, "Включение вспышки должно быть да или нет!", "Markdown");
                      Serial.println("Включение вспышки должно быть да или нет!");
                    }
                  }
                }
              }
            } else {
              bot.sendMessage(chat_id, "Ваш chat id не совпадает!", "Markdown");
            }
          }
          numNewMessages = bot.getUpdates(bot.last_message_received + 1);
        }
        last_message = millis(); // записываем время получения последнего сообщения
      }

      if (flash_state == true) { // если вспышка включена
        if (millis() - flash_on_millis > 60000) { // если вспышка включена больше 1 минуты
          flash_on_millis = millis(); // записываем время включения отправки предупреждения о долгой работе вспышки
          bot.sendMessage(chat_id, "Предупреждаю! При включении вспышки плата ESP32-CAM может перегреться", "Markdown");
          Serial.println("Предупреждаю! При включении вспышки плата ESP32-CAM может перегреться");
        }
      }

      if (send_photo_call == true) { // если есть вызов отправить фотографию
        Serial.println("Получен вызов сделать фотографию!");
        if (flash_onoff == "1") { // если включение вспышки при фотографировании установлено
          digitalWrite(flash_led_pin, HIGH); // включаем вспышку
        }
        sendPhoto(); // отправляем фотографию Telegram пользователю
        if (send_photo_return == "Не удалось сделать фотографию") { // если не удалось получить фотографию
          bot.sendMessage(chat_id, "Не удалось сделать фотографию :(", "Markdown");
        }
        if (send_photo_return == "Не удалось подключиться к api.telegram.org") { // если не удалось подключиться к Telegram api
          bot.sendMessage(chat_id, "Не удалось подключиться к api.telegram.org :(", "Markdown");
        }
        if (send_photo_return == "Не удалось отправить фотографию") { // если не удалось отправить фотографию
          bot.sendMessage(chat_id, "Не удалось отправить фотографию :(", "Markdown");
        }
        send_photo_call = false; // отключаем вызов отправки фотографии
        digitalWrite(flash_led_pin, LOW); // отключаем вспышку
      }

      if (start_stream == true) { // если получен вызов запустить видео поток на HTTP сервер
        httpd_config_t config = HTTPD_DEFAULT_CONFIG(); // настраиваем HTTP сервер
        config.server_port = 80; // настраиваем порт HTTP сервера

        // настраиваем HTTP метод и функцию для видео потока
        httpd_uri_t index_uri = {
          .uri       = "/",            // указываем HTTP uri
          .method    = HTTP_GET,       // указываем HTTP метод
          .handler   = stream_handler, // указываем функцию для видео потока
          .user_ctx  = NULL            // указываем указатель на данные контента пользователя
        };

        if (httpd_start(&stream_httpd, &config) == ESP_OK) { // если видео поток на HTTP сервер запущен
          httpd_register_uri_handler(stream_httpd, &index_uri); // регистрируем функцию для видео потока на HTTP сервер
          server_onoff = true; // записываем то, что видео поток на HTTP сервере запущен
          bot.sendMessage(chat_id, "Видео поток запущен! Переходите на http://" + ESP32IP, "Markdown");
          Serial.println("Видео поток запущен! Переходите на http://" + String(ESP32IP));
        } else {
          bot.sendMessage(chat_id, "Не удалось запустить видео поток :(", "Markdown");
          Serial.println("Не удалось запустить видео поток :(");
        }
        start_stream = false; // отключаем вызов запустить видео поток на HTTP сервере
      }

      if (end_stream == true) { // если получен вызов остановить видео поток на HTTP сервер
        if (stream_httpd != NULL) { // если HTTP сервер настроен 
          httpd_stop(stream_httpd); // останавливаем видео поток на HTTP сервере
          server_onoff = false; // записываем то, что видео поток на HTTP сервере остановлен
          bot.sendMessage(chat_id, "Видео поток остановлен!", "Markdown");
          Serial.println("Видео поток остановлен!");
        } else {
          bot.sendMessage(chat_id, "Не удалось остановить видео поток :(", "Markdown");
          Serial.println("Не удалось остановить видео поток :(");
        }
        end_stream = false; // отключаем вызов остановить видео поток на HTTP сервере
      }

      if (start_alarm == true) { // если отправка фотографий при обнаружении движения установлена
        if (pir_state == true) { // если обнаружено движение
          if (millis() - timing > motion_delay.toInt()) { // делаем задержку при обнаружении движения
            timing = millis(); // записываем время обнаружения движения
            send_photo_call = true; // записываем вызов отправить фотографию
            Serial.println("Обнаружено движение!");
          }
        }
      }
    } 
  }
}
