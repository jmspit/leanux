#!/bin/bash

tag="$1"
if [ -z ${tag} ]; then
  echo "you must specify a release tag argument"
  exit 1
fi

git tag -d "${tag}"
git push origin :refs/tags/"${tag}"
git tag -a "${tag}"  -m "release ${tag}"
git push origin "${tag}"
