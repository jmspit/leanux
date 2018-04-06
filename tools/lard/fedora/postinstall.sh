# create user and group @LARD_USER@:
/usr/bin/getent group @LARD_USER@ > /dev/null || /usr/sbin/groupadd -r @LARD_USER@
/usr/bin/getent passwd @LARD_USER@ > /dev/null || /usr/sbin/useradd -r -d /bin/lard -s /sbin/nologin -g @LARD_USER@ @LARD_USER@

# set permission on lard init script
/usr/bin/chmod 700 @LARD_SYSVINIT_FILE@

# set permission on lard config file
/usr/bin/chmod 660 @LARD_SYSCONF_FILE@

# set ownership on lard config dir
/usr/bin/chown -R root:@LARD_USER@ @LARD_SYSCONF_DIR@

# setup lard database dir
mkdir -p /var/lib/lard
touch /var/lib/lard/lard.db
chown -R @LARD_USER@:@LARD_USER@ /var/lib/lard
chmod 770 /var/lib/lard
chmod 660 /var/lib/lard/lard.db

# reload systemctl daemon status
/usr/bin/systemctl daemon-reload

# some info to the installing user
/usr/bin/echo "start lard on boot: /usr/sbin/chkconfig --add lard"
/usr/bin/echo "start lard now: /usr/sbin/service lard start"
