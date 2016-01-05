#!/bin/bash

export NODE_ENV=development
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DC="$DIR/docker-compose.yml"
PATH=$PATH:$DIR/.bin/
COMPOSE=$(which docker-compose)

if [ -z "$NODE_VER" ]; then
  NODE_VER="5.3.0"
fi

if ! [ -z "$TRAVIS_NODE_VERSION" ]; then
  NODE_VER=$(node --version)
  NODE_VER=${NODE_VER:1}
fi

if ! [ -x "$COMPOSE" ]; then
  mkdir $DIR/.bin
  curl -L https://github.com/docker/compose/releases/download/1.5.2/docker-compose-`uname -s`-`uname -m` > $DIR/.bin/docker-compose
  chmod +x $DIR/.bin/docker-compose
  COMPOSE=$(which docker-compose)
fi

function finish {
  $COMPOSE -f $DC stop
  $COMPOSE -f $DC rm -f
}
trap finish EXIT

export IMAGE=makeomatic/alpine-node:$NODE_VER
$COMPOSE -f $DC up -d
$COMPOSE -f $DC run --rm tester ./node_modules/.bin/mocha --harmony_destructuring
