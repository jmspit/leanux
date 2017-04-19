#/bin/bash
# use buildpkg.sh release
# ./buildpkg.sh 1.0.0

LEANUX_RELEASE=${1}
CLONE_DIR=/home/spjm/leanux-clone

VMLIST="centos debian fedora opensuse ubuntu"
#VMLIST="centos"
BUILDUSER="spjm"
PUBLISH="publish.html"
WEBUSER="root"
WEBSERVER="vlerk"

VBOX="/usr/bin/VBoxManage"

function packageExtension() {
  case ${1} in
    debian)
      echo "deb"
      return 0
      ;;
    ubuntu)
      echo "deb"
      return 0
      ;;
    archlinux)
      echo "xz"
      return 0;
      ;;
    *)
      echo "rpm"
      return 0
      ;;
  esac
  return 1
}

function startvm() {
  HOST=${1}
  UUID=${2}
  ${VBOX} startvm "${HOST}" --type headless || return 1
  return 0
}

function stopvm() {
  HOST=${1}
  UUID=${2}
  ${VBOX} controlvm "${HOST}" acpipowerbutton || return 1
  WAIT=1
  RUNNING=0
  MAXTRY=0
  while [ ${WAIT} -ne 0 ]
  do
    RUNNING=$(ps -ef | grep "VBoxHeadless --comment ${HOST} --startvm" | grep -v grep | wc -l)
    if [ ${RUNNING} -ne 0 ]
    then
      sleep 2
    else
      WAIT=0
    fi
    MAXTRY=$(expr ${MAXTRY} \+ 1)
    if [ ${MAXTRY} -ge 15 ]
    then
      WAIT=0
    fi
  done
  if [ ${RUNNING} -ne 0 ]
  then
    ${VBOX} controlvm "${HOST}" poweroff
  fi
  return 0
}

function copySource() {
  HOST=${1}
  SRC=${2}
  WAIT=1
  while [ ${WAIT} -eq 1 ]
  do
    scp ${SRC} ${BUILDUSER}@${HOST}:~
    if [ $? -ne 0 ]
    then
      sleep 5
    else
      WAIT=0
    fi
  done
  return 0
}

function unpackSource() {
  HOST=${1}
  SRC=${2}
  ssh ${BUILDUSER}@${HOST} "rm -rf leanux-${LEANUX_RELEASE}; tar zxvf ${SRC}" || return 1
  return 0;
}

function buildSource() {
  HOST=${1}
  SRC=${2}
  ssh ${BUILDUSER}@${HOST} "mkdir -p ${SRC%.tar.gz}/build/release" || return 1
  ssh ${BUILDUSER}@${HOST} "cd ${SRC%.tar.gz}/build/release && cmake ../.. -DCMAKE_BUILD_TYPE=Release && make package && sync" || return 1
  return 0;
}

function buildSourceArch() {
  HOST=${1}
  SRC=${2}
  ssh ${BUILDUSER}@${HOST} "rm ${SRC%.tar.gz}*.tar.xz; rm -rf src pkg" || return 1
  ssh ${BUILDUSER}@${HOST} "mkdir -p ${SRC%.tar.gz}/build/release" || return 1
  ssh ${BUILDUSER}@${HOST} "cd ${SRC%.tar.gz}/build/release && cmake ../.. -DCMAKE_BUILD_TYPE=Release && sync" || return 1
  ssh ${BUILDUSER}@${HOST} "cp ${SRC%.tar.gz}/build/release/${SRC%.tar.gz}.PKGBUILD ~" || return 1
  ssh ${BUILDUSER}@${HOST} "cp ${SRC%.tar.gz}/build/release/leanux.install ~" || return 1
  ssh ${BUILDUSER}@${HOST} "makepkg --skipinteg -p ${SRC%.tar.gz}.PKGBUILD && sync" || return 1
  return 0;
}

function copyPackages() {
  HOST=${1}
  SRC=${2}
  LOCALDIR=remotebuilds/${HOST}
  mkdir -p ${LOCALDIR}
  EXT=$(packageExtension ${HOST})
  scp ${BUILDUSER}@${HOST}:~/${SRC%.tar.gz}/build/release/*.${EXT} ${LOCALDIR} || return 1
  return 0;
}

function copyPackagesArch() {
  HOST=${1}
  SRC=${2}
  LOCALDIR=remotebuilds/${HOST}
  mkdir -p ${LOCALDIR}
  EXT=$(packageExtension ${HOST})
  scp ${BUILDUSER}@${HOST}:~/${SRC%.tar.gz}*.pkg.tar.${EXT} ${LOCALDIR} || return 1
  return 0;
}

# input sanity
if [ ! -z ${LEANUX_RELEASE} ]; then
  if [[ ! ${LEANUX_RELEASE} =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    (>&2 echo "invalid release '${LEANUX_RELEASE}' regexp should match ^[0-9]+\.[0-9]+\.[0-9]+$")
    exit 1
  fi
else
  (>&2 echo "specify a release number")
  exit 1
fi

#clone from git
test -d "${CLONE_DIR}" && rm -rf "${CLONE_DIR}"
mkdir -p "${CLONE_DIR}" || { echo "failed to create directory ${CLONE_DIR}"; exit 1; }
cd "${CLONE_DIR}" || { echo "failed to chdir into ${CLONE_DIR}"; exit 1; }
git clone https://github.com/jmspit/leanux.git || { echo "git clone failed"; exit 1; }
cd leanux || { echo "git clone 'leanux' found"; exit 1; }
git checkout -b ${LEANUX_RELEASE} || { echo "failed to checkout ${LEANUX_RELEASE}"; exit 1; }
SRC_DIR=${CLONE_DIR}/leanux/build/release
mkdir -p ${SRC_DIR} || { echo "failed to create directory ${SRC_DIR}"; exit 1; }
cd ${SRC_DIR} || { echo "failed to chdir into ${SRC_DIR}"; exit 1; }
cmake ../.. || { echo "cmake failed"; exit 1; }
make package_source || { echo "make package_source failed"; exit 1; }

SRC_FILE=${SRC_DIR}/leanux-${LEANUX_RELEASE}.tar.gz
SRC_SHORT=leanux-${LEANUX_RELEASE}.tar.gz

# build and retrieve the packages
for vm in ${VMLIST}
do
  startvm ${vm} || { echo "failed to start vm ${vm}"; exit 1; }
  copySource ${vm} ${SRC_FILE} || { echo "failed to copy source ${SRC_FILE} to vm ${vm}"; exit 1; }
  unpackSource ${vm} ${SRC_SHORT} || { echo "unpacking source ${SRC_SHORT} failed on vm ${vm}"; exit 1; }
  buildSource ${vm} ${SRC_SHORT} || { echo "building source ${SRC_SHORT} failed on vm ${vm}"; exit 1; }
  copyPackages ${vm} ${SRC_SHORT} || { echo "retrieving packages from vm ${vm} failed"; exit 1; }
  stopvm ${vm} || { echo "failed to stop vm ${vm}"; exit 1; }
done

# and seperately for special case arch linux
vm="archlinux"
startvm ${vm} || { echo "failed to start vm ${vm}"; exit 1; }
copySource ${vm} ${SRC_FILE} || { echo "failed to copy source ${SRC_FILE} to vm ${vm}"; exit 1; }
unpackSource ${vm} ${SRC_SHORT} || { echo "unpacking source ${SRC_SHORT} failed on vm ${vm}"; exit 1; }
buildSourceArch ${vm} ${SRC_SHORT} || { echo "building source ${SRC_SHORT} failed on vm ${vm}"; exit 1; }
copyPackagesArch ${vm} ${SRC_SHORT} || { echo "retrieving packages from vm ${vm} failed"; exit 1; }
stopvm ${vm} || { echo "failed to stop vm ${vm}"; exit 1; }


# publish
echo "<table>" > "${PUBLISH}"
for vm in archlinux ${VMLIST}
do
  LOCALDIR=remotebuilds/${vm}
  REMOTEDIR=/var/www/www.o-rho.com/htdocs/drupal/sites/default/files/article_files
  EXT=$(packageExtension ${vm})
  scp ${LOCALDIR}/*.${EXT} root@vlerk:${REMOTEDIR} || { echo "copy to website for distribution ${vm} failed"; exit 1; }
  ssh root@vlerk "chown root:apache ${REMOTEDIR}/*.${EXT}" || { echo "chown on website for distribution ${vm} failed"; exit 1; }
  ssh root@vlerk "chmod 640 ${REMOTEDIR}/*.${EXT}" || { echo "chmod on website for distribution ${vm} failed"; exit 1; }

  echo "<tr><th>${vm}</th><th>build date</th><th>size</th></tr>" >> ${PUBLISH}
  for file in $(ls -1 ${LOCALDIR})
  do
    DATE=$(stat -c %y ${LOCALDIR}/${file})
    SIZE=$(stat -c %s ${LOCALDIR}/${file})
    SHA256SUM=$(sha256sum ${LOCALDIR}/${file} | cut -d\  -f1)
    echo "<tr><td><a href=\"https://www.o-rho.com/sites/default/files/article_files/${file}\" title=\"$SHA256SUM\">${file}</a>" >> ${PUBLISH}
    echo "<td>$DATE</td><td>$SIZE</td></tr>" >> ${PUBLISH}
  done
done
echo "</table>" >> ${PUBLISH}

echo "build complete, build information written to $(pwd)/${PUBLISH}"
