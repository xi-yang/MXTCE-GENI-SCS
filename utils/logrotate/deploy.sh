#!/bin/bash
#deploy logrotate by doing the following:
cp logrotate4mxtce.conf /usr/local/etc/
cp logrotate4mxtce.cron /etc/cron.hourly/
chmod u+x /etc/cron.hourly/logrotate4mxtce.cron
