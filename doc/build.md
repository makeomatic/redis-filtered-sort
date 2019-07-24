# Build Redis FilterSortModule
Module works with `redis` v4.0 or higher.
## General Requirements
Cloned module [redis-filtered-sort](https://github.com/makeomatic/redis-filtered-sort) repository
```console
$~: git clone https://github.com/makeomatic/redis-filtered-sort.git
```

### Local build dependencies
* For local build you will need general C compiler and linker.
Generally on Ubuntu or other Debian like distros, all you have to do is install `build-essential` meta-package 
    ```console
    $~: sudo apt-get install build-essential
    ```

### Docker based build
For docker based build, You will need `docker` and `docker-compose` installed. This variant able generate special docker image with module included, or compile module under `alpine` linux.

## Local build
Enter `csrc` folder, located inside cloned repository.
Call:
```console
$~: make deps && make
```
Compiled module will be located `csrc/redis-filtered-sort/filter_module.so`. Copy this file into desired location and add `loadmodule {pathto filter_module.so}` option into your `redis.conf`.

## Docker build
Enter `csrc` folder, located inside cloned repository.
### Compile module
Set desired image name in `csrc\Makefile` and run
```
$~: make docker-build
```
If everything goes ok, You'll see **$DOCKER_IMAGE** image in your local *docker images*

If you want different name for image use `make docker-build IMAGE_NAME=foo/name:tag`

Push your image to docker with command

```
$~: make docker-push
```
