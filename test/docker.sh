#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DC="$DIR/docker-compose.yml"
PATH=$PATH:$DIR/.bin/
COMPOSE="docker-compose -f $DC"
$COMPOSE build redis
$COMPOSE up -d
$COMPOSE exec tester ./node_modules/.bin/mocha
