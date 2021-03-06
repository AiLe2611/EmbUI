#include "main.h"

#include "EmbUI.h"
#include "interface.h"

#include "uistrings.h"   // non-localized text-strings
#include "i18n.h"        // localized GUI text-strings

uint8_t lang = LANG::RU;   // default language for text resources

/**
 * Define configuration variables and controls handlers
 * variables has literal names and are kept within json-configuration file on flash
 * 
 * Control handlers are bound by literal name with a particular method. This method is invoked
 * by manipulating controls
 * 
 */
void create_parameters(){
    LOG(println, F("Создание конфигурационных переменных по-умолчанию"));

    embui.var_create(FPSTR(P_hostname), "");                // device hostname (autogenerated on first-run)
    embui.var_create(FPSTR(P_APonly),  FPSTR(P_false));     // режим AP-only (только точка доступа), не трогать
    embui.var_create(FPSTR(P_APpwd), "");                   // пароль внутренней точки доступа

    // параметры подключения к MQTT
    embui.var_create(FPSTR(P_m_host), "");                   // MQTT server hostname
    embui.var_create(FPSTR(P_m_port), F("1883"));            // MQTT port

    embui.var_create(FPSTR(P_m_user), "");                   // MQTT login
    embui.var_create(FPSTR(P_m_pass), "");                   // MQTT pass

    embui.var_create(FPSTR(P_m_pref), embui.mc);             // MQTT topic == use ESP MAC address
    embui.var_create(FPSTR(T_MPERIOD), "30");            // интервал отправки данных по MQTT в секундах (параметр в энергонезависимой памяти)

    // date/time related vars
    embui.var_create(FPSTR(P_TZSET), "");                   // TimeZone/DST rule (empty value == GMT/no DST)
    embui.var_create(FPSTR(P_userntp), "");                 // Backup NTP server

    embui.var_create(FPSTR(T_LANGUAGE), String((uint8_t)lang));     // language

    /**
     * обработчики действий
     */ 
    // вывод WebUI секций
    embui.section_handle_add(FPSTR(T_SETTINGS), section_settings_frame);    // generate "settings" UI section
    embui.section_handle_add(FPSTR(T_SH_NETW), block_settings_netw);        // generate "network settings" UI section
    embui.section_handle_add(FPSTR(T_SH_TIME), block_settings_time);         // generate "time settings" UI section
    //embui.section_handle_add(FPSTR(T_SH_OTHER), show_settings_other);

    // обработка базовых настроек
    embui.section_handle_add(FPSTR(T_SET_WIFI), set_settings_wifi);         // обработка настроек WiFi Client
    embui.section_handle_add(FPSTR(T_SET_WIFIAP), set_settings_wifiAP);     // обработка настроек WiFi AP
    embui.section_handle_add(FPSTR(T_SET_MQTT), set_settings_mqtt);         // обработка настроек MQTT
    embui.section_handle_add(FPSTR(T_SET_TIME), set_settings_time);
    embui.section_handle_add(FPSTR(T_LANGUAGE), set_language);

    //embui.section_handle_add(FPSTR(T_004B), set_settings_other);
}


/**
 * Headlile section
 * this is an overriden weak method that builds our WebUI from the top
 * ==
 * Головная секция
 * переопределенный метод фреймфорка, который начинает строить корень нашего WebUI
 * 
 */
void section_main_frame(Interface *interf, JsonObject *data){
    if (!interf) return;

    interf->json_frame_interface(FPSTR(T_HEADLINE));  // HEADLINE for WebUI

    block_menu(interf, data);                       // Строим UI блок с меню выбора других секций

    if(!embui.sysData.wifi_sta){                // если контроллер не подключен к внешней AP, открываем вкладку настройки WiFi 
        block_settings_netw(interf, data);
    } else {
        //block_demo(interf, data);       // Строим блок с demo переключателями
    }

    //block_more(interf, data);                     // у нас есть и другие блоки, но строить сразу все
    //block_setup(interf, data);                    // не требуеся. Будем переходить по меню далее

    interf->json_frame_flush();
}

/**
 * This code builds UI section with menu block on the left
 * 
 */
void block_menu(Interface *interf, JsonObject *data){
    if (!interf) return;
    // создаем меню
    embui.autoSaveReset(); // автосохранение конфига будет отсчитываться от этого момента
    interf->json_section_menu();

    //interf->option(FPSTR(T_DEMO), FPSTR(T_DICT[lang][TD::D_OTHER));
    //interf->option(FPSTR(T_MORE), FPSTR(T_DICT[lang][TD::D_MORE]));
    interf->option(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));     // пункт меню "настройки"
    interf->json_section_end();
}

/**
 * формирование секции "настроек",
 * вызывается либо по выбору из "меню" либо при вызове из
 * других блоков/обработчиков
 * 
 */
void section_settings_frame(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface(FPSTR(T_HEADLINE));

    interf->json_section_main(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_SETTINGS]));

    interf->select(FPSTR(T_LANGUAGE), String(lang), String(FPSTR(T_DICT[lang][TD::D_LANG])), true);
    interf->option("0", "Rus");
    interf->option("1", "Eng");
    interf->json_section_end();

    interf->spacer();

    interf->button(FPSTR(T_SH_NETW), FPSTR(T_DICT[lang][TD::D_WIFI_MQTT]));  // кнопка перехода в настройки сети
    interf->button(FPSTR(T_SH_TIME), FPSTR(T_DICT[lang][TD::D_Time]));       // кнопка перехода в настройки времени
    interf->button(FPSTR(T_SH_OTHER), FPSTR(T_DICT[lang][TD::D_OTHER]));     // кнопка перехода в другие настройки

    interf->spacer();
    block_settings_update(interf, data);                                   // вызываем блок интерфейса обновления ПО

    interf->json_section_end();
    interf->json_frame_flush();
}

/**
 *  WebUI блок интерфейса настроек WiFi/MQTT
 */
void block_settings_netw(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();

    // Headline
    interf->json_section_main(FPSTR(T_OPT_NETW), FPSTR(T_DICT[lang][TD::D_WIFI_MQTT]));

    // форма настроек Wi-Fi Client
    interf->json_section_hidden(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_WiFiClient]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiClientOpts]));
    interf->text(FPSTR(P_hostname), FPSTR(T_DICT[lang][TD::D_Hostname]));
    interf->text(FPSTR(T_WCSSID), WiFi.SSID(), FPSTR(T_DICT[lang][TD::D_WiFiSSID]), false);

/*
    String a;
    if (WiFi.getMode()==WIFI_AP_STA)
        a="zzzz";
*/

    //WiFi.getMode()==WIFI_AP_STA ? "ZZZZ" : ""
//    interf->text(FPSTR(T_WCSSID), "", FPSTR(T_DICT[lang][TD::D_WiFiSSID]), false);
    interf->password(FPSTR(T_WCPASS), FPSTR(T_DICT[lang][TD::D_Password]));
    interf->button_submit(FPSTR(T_SET_WIFI), FPSTR(T_DICT[lang][TD::D_CONNECT]), FPSTR(T_GRAY));
    interf->json_section_end();

    // форма настроек Wi-Fi AP
    interf->json_section_hidden(FPSTR(T_SET_WIFIAP), FPSTR(T_DICT[lang][TD::D_WiFiAP]));
    interf->text(FPSTR(P_hostname), FPSTR(T_DICT[lang][TD::D_Hostname]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_WiFiAPOpts]));
    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_APOnly]));
    interf->checkbox(FPSTR(P_APonly), FPSTR(T_DICT[lang][TD::D_APOnlyMode]));
    interf->password(FPSTR(P_APpwd),  FPSTR(T_DICT[lang][TD::D_MSG_APProtect]));
    interf->button_submit(FPSTR(T_SET_WIFIAP), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(T_GRAY));
    interf->json_section_end();

    // форма настроек MQTT
    interf->json_section_hidden(FPSTR(T_SET_MQTT), FPSTR(T_DICT[lang][TD::D_MQTT]));
    interf->text(FPSTR(P_m_host), FPSTR(T_DICT[lang][TD::D_MQTT_Host]));
    interf->number(FPSTR(P_m_port), FPSTR(T_DICT[lang][TD::D_MQTT_Port]));
    interf->text(FPSTR(P_m_user), FPSTR(T_DICT[lang][TD::D_User]));
    interf->text(FPSTR(P_m_pass), FPSTR(T_DICT[lang][TD::D_Password]));
    interf->text(FPSTR(P_m_pref), FPSTR(T_DICT[lang][TD::D_MQTT_Topic]));
    interf->number(FPSTR(T_MPERIOD), FPSTR(T_DICT[lang][TD::D_MQTT_Interval]));
    interf->button_submit(FPSTR(T_SET_MQTT), FPSTR(T_DICT[lang][TD::D_CONNECT]), FPSTR(T_GRAY));
    interf->json_section_end();

    interf->spacer();
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_section_end();

    interf->json_frame_flush();
}

/**
 *  WebUI блок загрузки обновлений ПО
 */
void block_settings_update(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_section_hidden(FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_Update]));
    interf->spacer(FPSTR(T_DICT[lang][TD::D_FWLOAD]));
    interf->file(FPSTR(T_DO_OTAUPD), FPSTR(T_DO_OTAUPD), FPSTR(T_DICT[lang][TD::D_UPLOAD]));
}

/**
 *  WebUI блок настройки даты/времени
 */
void block_settings_time(Interface *interf, JsonObject *data){
    if (!interf) return;
    interf->json_frame_interface();
    interf->json_section_main(FPSTR(T_SET_TIME), FPSTR(T_DICT[lang][TD::D_DATETIME]));

    interf->comment(FPSTR(T_DICT[lang][TD::D_MSG_TZSet01]));     // комментарий-описание секции
    interf->text(FPSTR(P_TZSET), FPSTR(T_DICT[lang][TD::D_MSG_TZONE]));
    interf->text(FPSTR(P_userntp), FPSTR(T_DICT[lang][TD::D_NTP_Secondary]));
    interf->text(FPSTR(T_DTIME), FPSTR(T_DICT[lang][TD::D_MSG_DATETIME]));
    interf->button_submit(FPSTR(T_SET_TIME), FPSTR(T_DICT[lang][TD::D_SAVE]), FPSTR(T_GRAY));

    interf->spacer();
    interf->button(FPSTR(T_SETTINGS), FPSTR(T_DICT[lang][TD::D_EXIT]));

    interf->json_section_end();
    interf->json_frame_flush();
}

/**
 * Обработчик настроек WiFi в режиме клиента
 */
void set_settings_wifi(Interface *interf, JsonObject *data){
    if (!data) return;

    SETPARAM(FPSTR(P_hostname));        // сохраняем hostname в конфиг

    const char *ssid = (*data)[FPSTR(T_WCSSID)];    // переменные доступа в конфиге не храним
    const char *pwd = (*data)[FPSTR(T_WCPASS)];     // фреймворк хранит последнюю доступную точку самостоятельно

    if(ssid){
        embui.wifi_connect(ssid, pwd);
    } else {
        LOG(println, F("WiFi: No SSID defined!"));
    }

    section_settings_frame(interf, data);           // переходим в раздел "настройки"
}

/**
 * Обработчик настроек WiFi в режиме AP
 */
void set_settings_wifiAP(Interface *interf, JsonObject *data){
    if (!data) return;

    SETPARAM(FPSTR(P_hostname));    // эти переменные будут сохранены в конфиг-файл
    SETPARAM(FPSTR(P_APonly));
    SETPARAM(FPSTR(P_APpwd));

    embui.save();
    embui.wifi_connect();           // иницируем WiFi-подключение с новыми параметрами

    section_settings_frame(interf, data);   // переходим в раздел "настройки"
}

/**
 * Обработчик настроек MQTT
 */
void set_settings_mqtt(Interface *interf, JsonObject *data){
    if (!data) return;
    // сохраняем настройки в конфиг
    SETPARAM(FPSTR(P_m_host));
    SETPARAM(FPSTR(P_m_port));
    SETPARAM(FPSTR(P_m_user));
    SETPARAM(FPSTR(P_m_pass));
    SETPARAM(FPSTR(P_m_pref));
    //SETPARAM(FPSTR(T_MPERIOD), some_mqtt_object.semqtt_int((*data)[FPSTR(T_MPERIOD)]));

    embui.save();

    section_settings_frame(interf, data);
}

/**
 * Обработчик настроек даты/времени
 */
void set_settings_time(Interface *interf, JsonObject *data){
    if (!data) return;

    String datetime=(*data)[FPSTR(T_DTIME)];
    if (datetime.length())
        embui.timeProcessor.setTime(datetime);
    SETPARAM(FPSTR(P_TZSET), embui.timeProcessor.tzsetup((*data)[FPSTR(P_TZSET)]));
    SETPARAM(FPSTR(P_userntp), embui.timeProcessor.setcustomntp((*data)[FPSTR(P_userntp)]));

    section_settings_frame(interf, data);
}

void set_language(Interface *interf, JsonObject *data){
        if (!data) return;

    //lang = (*data)[FPSTR(T_LANGUAGE)].as<unsigned char>();
    SETPARAM(FPSTR(T_LANGUAGE), lang = (*data)[FPSTR(T_LANGUAGE)].as<unsigned char>() );

    section_settings_frame(interf, data);
}

void pubCallback(Interface *interf){
    if (!interf) return;
    interf->json_frame_value();
    interf->value(F("pTime"), embui.timeProcessor.getFormattedShortTime(), true);
    interf->value(F("pMem"), String(ESP.getFreeHeap()), true);
    interf->value(F("pUptime"), String(millis()/1000), true);
    interf->json_frame_flush();
}
