#!/bin/bash

###
# build selected dockerfile and tag it based in labels
##

set -o errexit

while getopts "t:v:a:c:f:p:l:r:h" opt; do
    case $opt in
        v)
          REDIS_VERSION=$OPTARG
          ;;
        a)
          ALPINE_VERSION=$OPTARG
          ;;
        t)
          _tempname=$OPTARG
          ;;
        r)
          docker_repo=$OPTARG
          ;;
        p)
          push=true
          ;;
        c)
          cache=$OPTARG
          ;;
        l)
          appendix=$OPTARG
          ;;
        f)
          file=$OPTARG
          ;;
        h)
          help=true
    esac
done

REDIS_VERSION="${REDIS_VERSION:-5.0.5}"
ALPINE_VERSION="${ALPINE_VERSION:-3.10}"
file="${file:-./Dockerfile}"
docker_repo="${docker_repo:-makeomatic}"

if [ ! -z "$help" ]; then
  echo "
USAGE: cmd -f ./Dockerfile -v 1.12.0 [-p postfix] [-c cache-tag]
  current script generates set of images based on source LABEL version_tags
  arguments:
  -f: path to Dockerfile
  -v: redis version
  -a: alpine version - alpine version should match redis image alpine version
  -l: version postfix, if not set - additional 'latest' tag will be generated
  -p: push build image to remote repo
  -t: custom temporary image name, it won't be deleted, useful for tests
  -c: image with layer to reuse, useful for speeding up the builds
"
  exit 1
fi

### detect package version
MODULE_VERSION=$(sed -n -e "s/^.*\"version\":.*\"\(.*\)\",/\1/p" < package.json)

image="${docker_repo}/redis-fsort"
tempname=${_tempname:-`cat /dev/urandom | LC_CTYPE=C tr -dc 'a-z0-9' | fold -w 32 | head -n 1`}
versions_label="module_version"
cachename="$image:$cache"

### generate docker image with ENV vars fulfilled
tmpfile=$(mktemp /tmp/dockerfile.XXXXXX)

export MODULE_VERSION
export REDIS_VERSION
export ALPINE_VERSION

echo "==================================================="
echo "Redis: ${REDIS_VERSION}; Alpine: ${ALPINE_VERSION}"
echo "Dockerfile: ${file}; Repository: ${docker_repo}"
echo "Fsort Version: ${MODULE_VERSION}"
echo "==================================================="

# we specify here exact variables to replace so envsubst won't touch the rest
envsubst '${MODULE_VERSION} ${ALPINE_VERSION} ${REDIS_VERSION}' < $file > $tmpfile

### create temp container
[ ! -z "$cache" ] && docker pull $cachename

buildArgs=("--tag $tempname" "--file $tmpfile")
[ ! -z "$cache" ] && buildArgs+=("--cache-from $cachename")
buildArgsStr=$(printf " %s" "${buildArgs[@]}")
echo "BuildArgs: ${buildArgsStr}"
docker build $buildArgsStr .
rm $tmpfile

### generate tags based on image labels
[ `uname` = "Darwin" ] && opts="-E" || opts="-r"
versions=$(docker inspect -f "{{.Config.Labels.$versions_label}}" $tempname | sed $opts -e 's/"|\[|\]//g' -e 's/,/ /g')

[ -z $appendix ] && versions+=("latest")

### tag images
for version in $versions; do
  tag=${REDIS_VERSION}-${version}-${appendix}
  tag=${tag%-}

  docker tag $tempname $image:$tag
  echo "==> tagged: $image:$tag"
done

### remove unwanted images if we not specify exact temp name (test purposes)
[ -z $_tempname ] &&  docker rmi $tempname

### push images to remote repository
if [ ! -z "$push" ]; then
  for version in $versions; do
    tag=${version}-${appendix}
    tag=${tag%-}
    docker push $image:$tag
  done
fi