from datetime import datetime, timedelta
import calendar
from typing import OrderedDict
import os

months_temp = [-11.6, -7.6, -1.0, 6., 10.7, 14.5, 18.8, 20.6, 16.7, 9.6, -0.1, -8.8]

def get_month_temp(t_month): # 0-11 == Jan-Dec
    month = int(t_month)
    start_month_temp = months_temp[month]
    end_month_temp = months_temp[0 if month > 10 else month + 1]
    
    t_this_month = t_month - month

    return start_month_temp * (1 - t_this_month) + end_month_temp * t_this_month

def get_day_temp(t_day):
    if (t_day < 0.25):
        return -4 - 2*(t_day*4)
    elif (t_day < 0.5):
        return -6 + 14 * (t_day - 0.25)*4
    elif (t_day < 0.75):
        return 8 - 4 * (t_day - 0.5)*4
    else:   
        return 4 - 8 * (t_day - 0.75)*4
    

today = datetime.now()
sec_ago = today - timedelta(seconds=1)
year_ago = today.replace(year=today.year - 1)
month_ago = today - timedelta(days=30)

log_dir = "./build/logs"
if not os.path.exists(log_dir):
    os.makedirs(log_dir)

# Generate all measurements log
day_log = OrderedDict()
day_gen_date = today - timedelta(days=1)
while day_gen_date < today:
    timestamp = int(datetime.timestamp(day_gen_date))
    t_day = (day_gen_date.hour * 3600 + day_gen_date.minute * 60 + day_gen_date.second) / 86400
    t_month = day_gen_date.month - 1 + (day_gen_date.day - 1) / calendar.monthrange(day_gen_date.year, day_gen_date.month)[-1]
    
    day_log[timestamp] = get_month_temp(t_month) + get_day_temp(t_day)
    day_gen_date += timedelta(seconds=1)

with open(os.path.join(log_dir, "temperature_log_all.log"), "w") as f:
    for d, t in day_log.items():
        f.writelines(f"{d} {t:.5f}\n")

# Generate hourly average log
hour_log = OrderedDict()
hour_gen_date = month_ago
while hour_gen_date < today:
    timestamp = int(datetime.timestamp(hour_gen_date))
    t_month = hour_gen_date.month - 1 + (hour_gen_date.day - 1) / calendar.monthrange(hour_gen_date.year, hour_gen_date.month)[-1]
    t_day = (hour_gen_date.hour * 3600) / 86400
    
    hour_log[timestamp] = get_month_temp(t_month) + get_day_temp(t_day)
    hour_gen_date += timedelta(hours=1)

with open(os.path.join(log_dir, "temperature_log_hourly.log"), "w") as f:
    for d, t in hour_log.items():
        f.writelines(f"{d} {t:.5f}\n")

# Generate daily average log
year_log = OrderedDict()
year_gen_date = year_ago
while year_gen_date < today:
    timestamp = int(datetime.timestamp(year_gen_date))
    t_month = year_gen_date.month - 1 + (year_gen_date.day - 1) / calendar.monthrange(year_gen_date.year, year_gen_date.month)[-1]
    
    year_log[timestamp] = get_month_temp(t_month)
    year_gen_date += timedelta(days=1)

with open(os.path.join(log_dir, "temperature_log_daily.log"), "w") as f:
    for d, t in year_log.items():
        f.writelines(f"{d} {t:.5f}\n")
