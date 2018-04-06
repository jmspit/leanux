# create user and group @LARD_USER@:
getent group @LARD_USER@ > /dev/null || groupadd -r @LARD_USER@
getent passwd @LARD_USER@ > /dev/null || useradd -r -M -s /sbin/nologin -g @LARD_USER@ @LARD_USER@

# set permission on lard init script
/usr/bin/chmod 700 @LARD_SYSVINIT_FILE@

# set permission on lard config file
chmod 660 @LARD_SYSCONF_FILE@

# setup lard database dir
mkdir -p /var/lib/lard
touch /var/lib/lard/lard.db
chown -R @LARD_USER@:@LARD_USER@ /var/lib/lard
chmod 770 /var/lib/lard
chmod 660 /var/lib/lard/lard.db

# reload systemctl daemon status
systemctl daemon-reload

# some info to the installing user
echo "start lard on boot: systemctl enable lard"
echo "start lard now: systemctl start lard"
