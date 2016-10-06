#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DC="$DIR/docker-compose.yml"
PATH=$PATH:$DIR/.bin/
COMPOSE="docker-compose -f $DC"

if ! [ -x $(which docker-compose) ]; then
  mkdir $DIR/.bin
  curl -L https://github.com/docker/compose/releases/download/1.8.1/docker-compose-`uname -s`-`uname -m` > $DIR/.bin/docker-compose
  chmod +x $DIR/.bin/docker-compose
fi

if [ x"$CI" = x"true" ]; then
  trap "$COMPOSE stop; $COMPOSE rm -f -v" EXIT
fi

$COMPOSE up -d
$COMPOSE exec tester ./node_modules/.bin/mocha
