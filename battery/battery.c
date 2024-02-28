#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>

#define DEBUG 0 

const char *PATH = "/sys/class/power_supply/BAT0/uevent";

typedef struct {
    char *status;
    unsigned int voltage_now;
    unsigned int current_now;
    unsigned int capacity_now;
    unsigned int charge_now;
    unsigned int max_charge;
    unsigned int initial_max_charge;
} Battery;

void init_battery(Battery *battery) {
    battery->status = NULL;
    battery->voltage_now = UINT32_MAX;
    battery->current_now = UINT32_MAX;
    battery->capacity_now = UINT32_MAX;
    battery->charge_now = UINT32_MAX;
    battery->max_charge = UINT32_MAX;
    battery->initial_max_charge = UINT32_MAX;
}

void free_battery(Battery *battery) {
    if (battery->status != NULL) free(battery->status);
}

int main() {
    FILE *f = fopen(PATH, "r");
    if (f == NULL) {
        int errkeep = errno;
        printf("Error while opening %s: %s", PATH, strerror(errkeep));
        return 1;
    }

    Battery battery;
    init_battery(&battery);

    char line[1024];
    const char *sep = "=";
    unsigned int hours, minutes;
    float cur_battery_level, seconds_remaining;

    char *ret = fgets(line, 1024, f);

    while (ret != NULL) {
        char *val = line;
        // Split the string over '=' char
        char *key = strsep(&val, sep);

        // Strip the newline on the end of string
        size_t len = strlen(val);
        if (len > 0) {
            val[len - 1] = '\0';
        }

        // Check keys/vals
        if (strcmp(key, "POWER_SUPPLY_STATUS") == 0) {
            battery.status = strdup(val);
        } else if (strcmp(key, "POWER_SUPPLY_CHARGE_NOW") == 0) {
            battery.charge_now = atol(val);
        } else if (strcmp(key, "POWER_SUPPLY_CHARGE_FULL") == 0) {
            battery.max_charge = atol(val);
        } else if (strcmp(key, "POWER_SUPPLY_CHARGE_FULL_DESIGN") == 0) {
            battery.initial_max_charge = atol(val);
        } else if (strcmp(key, "POWER_SUPPLY_CAPACITY") == 0) {
            battery.capacity_now = atoi(val);
        } else if (strcmp(key, "POWER_SUPPLY_CURRENT_NOW") == 0) {
            battery.current_now = atol(val);
        }

        if (DEBUG) {
            printf("%s = %s\n", key, val);
        }
        ret = fgets(line, 1024, f);
    }
    fclose(f); // the file is not needed anymore

    // compute a few numbers for display given we now have battery data available
    if (battery.max_charge == 0 || battery.current_now == 0) {
        fprintf(stderr, "Could not retrieve sufficient battery information");
        return 2;
    }
    cur_battery_level = 100. * battery.charge_now / battery.max_charge;
    seconds_remaining = 3600. * (battery.max_charge - battery.charge_now) / battery.current_now;
    if (seconds_remaining > 0) {
        hours = seconds_remaining / 3600;
        minutes = (seconds_remaining - (hours * 3600)) / 60;
    }

    // print out result, cleanup & exit
    printf("BAT: %.2f%% %02d:%02d", cur_battery_level, hours, minutes);
    free_battery(&battery);

    if (cur_battery_level > 20) {
        return 0;
    } else {
        return 1;
    }
}
