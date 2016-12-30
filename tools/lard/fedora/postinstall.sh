# create user and group leanux:
/usr/bin/getent group leanux > /dev/null || /usr/sbin/groupadd -r leanux
/usr/bin/getent passwd leanux > /dev/null || /usr/sbin/useradd -r -d /bin/lard -s /sbin/nologin -g leanux leanux

# set permission on lard init script
/usr/bin/chmod 700 /etc/init.d/lard

# set permission on lard config file
/usr/bin/chmod 660 /etc/lard/lard.conf

# set ownership on lard config dir
/usr/bin/chown -R root:leanux /etc/lard

# some info to the installing user
/usr/bin/echo "start lard on boot: /usr/sbin/chkconfig --add lard"
/usr/bin/echo "start lard now: /usr/sbin/service lard start"
