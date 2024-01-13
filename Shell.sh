#!/bin/bash

# Run the weather monitoring program 24 times (once per hour)
for i in {1..12}
do
  ./main  # Assuming your compiled program is named 'main'
  sleep 3600  # Sleep for 1 second
done

# Calculate average temperature and humidity
avg_temperature=$(grep "Temperature:" weather_report.txt | sed 's/[^0-9.-]//g' | awk '{ sum += $1 } END { if (NR > 0) print sum / NR }')
avg_humidity=$(grep "Humidity:" weather_report.txt | awk '{ sum += $2 } END { if (NR > 0) print sum / NR }')

# Append 24-hour report to the weather report file
echo -e "\n24 Hours Report:" >> weather_report.txt
echo "Average Temperature: $avg_temperature degrees Celsius" >> weather_report.txt
echo "Average Humidity: $avg_humidity%" >> weather_report.txt
