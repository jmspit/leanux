# create user and group leanux:
/usr/bin/getent group @LARD_USER@ > /dev/null || /usr/sbin/groupadd -r @LARD_USER@
/usr/bin/getent passwd @LARD_USER@ > /dev/null || /usr/sbin/useradd -r -d /bin/lard -s /sbin/nologin -g @LARD_USER@ @LARD_USER@

# set permission on lard init script
/usr/bin/chmod 700 @LARD_SYSVINIT_FILE@

# set permission on lard config file
/usr/bin/chmod 660 @LARD_SYSCONF_FILE@

# set ownership on lard config dir
/usr/bin/chown -R root:@LARD_USER@ @LARD_SYSCONF_DIR@

# register the daemon
/usr/bin/systemctl daemon-reload

# some info to the installing user
/bin/echo "start lard on boot: /usr/sbin/chkconfig --add lard"
/bin/echo "start lard now: /usr/sbin/service lard start"
