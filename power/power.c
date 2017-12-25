/*
 * Copyright (C) 2012 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <stdlib.h>

#include <utils/Log.h>
#include <cutils/log.h>
#include <cutils/properties.h>
#include <hardware/hardware.h>
#include <hardware/power.h>
#include <pthread.h>
#include <hardware/hardware.h>
#include <hardware/power.h>
//#include "power-feature.h"

#define LOG_TAG "MTK PowerHAL"

enum {
    PROFILE_POWER_SAVE,
    PROFILE_BALANCED,
    PROFILE_HIGH_PERFORMANCE,
};

#define POWER_NR_OF_SUPPORTED_PROFILES 3

//#define POWER_PROFILE_PROPERTY  "sys.perf.profile"
//#define POWER_SAVE_PROP         "0"
//#define BALANCED_PROP           "1"
//#define HIGH_PERFORMANCE_PROP   "2"
//#define BIAS_POWER_PROP         "3"

#define MT_RUSH_BOOST_PATH "/proc/hps/rush_boost_enabled"
#define MT_FPS_UPPER_BOUND_PATH "/d/ged/hal/fps_upper_bound"
#define CPU0_POWER_SAVE_MAX "/sys/devices/system/cpu/cpu0/cpufreq/scaling_max_freq"
#define CPU4_POWER_SAVE_MAX "/sys/devices/system/cpu/cpu4/cpufreq/scaling_max_freq"
#define CPU8_POWER_SAVE_MAX "/sys/devices/system/cpu/cpu8/cpufreq/scaling_max_freq"
#define CPU0_POWER_SAVE_MIN "/sys/devices/system/cpu/cpu0/cpufreq/scaling_min_freq"
#define CPU4_POWER_SAVE_MIN "/sys/devices/system/cpu/cpu4/cpufreq/scaling_min_freq"
#define CPU8_POWER_SAVE_MIN "/sys/devices/system/cpu/cpu8/cpufreq/scaling_min_freq"
#define CPU0_POWER_SAVE_GOVERNOR "/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor"
#define CPU4_POWER_SAVE_GOVERNOR "/sys/devices/system/cpu/cpu4/cpufreq/scaling_governor"
#define CPU8_POWER_SAVE_GOVERNOR "/sys/devices/system/cpu/cpu8/cpufreq/scaling_governor"
#define MT_PPM_MODE_PATH "/proc/ppm/mode"
#define MT_HPS_POWER_MODE_PATH "/proc/hps/power_mode"
#define MT_CPUFREQ_POWER_MODE_PATH "/proc/cpufreq/cpufreq_power_mode"
#define MT_GED_EVENT_NOTIFY_PATH "/d/ged/hal/event_notify"
#define MT_DISPSYS_PATH "/d/dispsys"

static int current_power_profile = PROFILE_BALANCED;

static int power_fwrite(const char *path, char *s)
{
    char buf[80];
    int len;
    int fd = open(path, O_WRONLY);

    if (fd < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error opening %s: %s\n", path, buf);
        return -1;
    }

    len = write(fd, s, strlen(s));
    if (len < 0) {
        strerror_r(errno, buf, sizeof(buf));
        ALOGE("Error writing to %s: %s\n", path, buf);
        return -1;
    }

    close(fd);
    return 0;
}

static void power_init(struct power_module *module __unused)
{
    ALOGI("%s", __func__);
}

static void power_set_interactive(struct power_module *module __unused,
                int on __unused)
{
}

static void set_power_profile(int profile)
{
    if (profile == current_power_profile)
        return;

    switch (profile) {
    case PROFILE_POWER_SAVE:
        ALOGI("POWER_HINT_POWER_SAVING");
            power_fwrite(MT_FPS_UPPER_BOUND_PATH, "30");
            power_fwrite(MT_RUSH_BOOST_PATH, "0");
            power_fwrite(MT_PPM_MODE_PATH, "low_power");
            power_fwrite(MT_HPS_POWER_MODE_PATH, "1");
            power_fwrite(MT_CPUFREQ_POWER_MODE_PATH, "1");
            power_fwrite(MT_GED_EVENT_NOTIFY_PATH, "low-power-mode 1");
            power_fwrite(MT_DISPSYS_PATH, "low_power_mode:1");

        break;
    case PROFILE_BALANCED:
        ALOGI("POWER_HINT_BALANCE");
            power_fwrite(MT_FPS_UPPER_BOUND_PATH, "60");
            power_fwrite(MT_RUSH_BOOST_PATH, "1");
            power_fwrite(MT_PPM_MODE_PATH, "performance");
            power_fwrite(MT_HPS_POWER_MODE_PATH, "3");
            power_fwrite(MT_CPUFREQ_POWER_MODE_PATH, "3");
            power_fwrite(MT_GED_EVENT_NOTIFY_PATH, "low-power-mode 0");
            power_fwrite(MT_DISPSYS_PATH, "low_power_mode:2");
        break;
    case PROFILE_HIGH_PERFORMANCE:
        ALOGI("POWER_HINT_PERFORMANCE_BOOST");
            power_fwrite(MT_FPS_UPPER_BOUND_PATH, "90");
            power_fwrite(MT_RUSH_BOOST_PATH, "1");
            power_fwrite(MT_PPM_MODE_PATH, "performance");
            power_fwrite(MT_HPS_POWER_MODE_PATH, "3");
            power_fwrite(MT_CPUFREQ_POWER_MODE_PATH, "3");
            power_fwrite(MT_GED_EVENT_NOTIFY_PATH, "low-power-mode 0");
            power_fwrite(MT_DISPSYS_PATH, "low_power_mode:3");
        break;
    //case PROFILE_BIAS_POWER:
    //    property_set(POWER_PROFILE_PROPERTY, BIAS_POWER_PROP);
    //    break;
    }

    current_power_profile = profile;
}


static void power_hint(struct power_module *module __unused, power_hint_t hint,
                void *data __unused)
{
    switch (hint) {
      case POWER_HINT_SET_PROFILE:
	  set_power_profile(*(int32_t *)data);
      default:
	  break;
    }
}

static void set_feature(struct power_module *module __unused,
                feature_t feature, int state)
{
    //set_device_specific_feature(module, feature, state);
}

static int get_feature(struct power_module *module __unused, feature_t feature)
{
    if (feature == POWER_FEATURE_SUPPORTED_PROFILES)
        return POWER_NR_OF_SUPPORTED_PROFILES;
    return -1;
}

static struct hw_module_methods_t power_module_methods = {
    .open = NULL,
};

struct power_module HAL_MODULE_INFO_SYM = {
    .common = {
        .tag = HARDWARE_MODULE_TAG,
        .module_api_version = POWER_MODULE_API_VERSION_0_3,
        .hal_api_version = HARDWARE_HAL_API_VERSION,
        .id = POWER_HARDWARE_MODULE_ID,
        .name = "MTK Power HAL",
        .author = "The Android Open Source Project",
        .methods = &power_module_methods,
    },

    .init = power_init,
    .setInteractive = power_set_interactive,
    .powerHint = power_hint,
    .setFeature = set_feature,
    .getFeature = get_feature
};
