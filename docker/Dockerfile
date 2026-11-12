# Entorno de compilación de ZXMetalCircle: todo lo que necesitan build.sh y las
# pruebas en el PC, ya instalado. La Action compila dentro de esta imagen, así
# que no depende de los servidores de Ubuntu ni de los de Arm en cada ejecución
# (solo de GitHub). La construye y la sube a GHCR la Action «Docker»
# (.github/workflows/docker.yml) cuando cambia algo de esta carpeta.
#
# También sirve para compilar en casa con Docker, sin instalar nada más:
#   docker run --rm -v "$PWD":/src ghcr.io/shaeon/zxmetalcircle-build:1 ./build.sh
#
# Si se cambia algo que afecte a la compilación (otro compilador, otro paquete),
# súbase el número de docker/VERSION y el de la imagen en build.yml.

FROM ubuntu:24.04

ARG ARM_TOOLCHAIN=14.2.rel1

ENV DEBIAN_FRONTEND=noninteractive

# Paquetes: g++ para las pruebas en el PC; git, wget, make y python3 para
# build.sh; mtools, dosfstools y zip para la tarjeta y su zip
RUN apt-get update \
 && apt-get install -y --no-install-recommends \
	build-essential make git ca-certificates wget xz-utils python3 \
	mtools dosfstools zip \
 && rm -rf /var/lib/apt/lists/*

# Arm GNU Toolchain: arm-none-eabi (32 bits) y aarch64-none-elf (64 bits)
RUN set -e; \
	mkdir -p /opt/arm-toolchain; \
	cd /opt/arm-toolchain; \
	base="https://developer.arm.com/-/media/Files/downloads/gnu/${ARM_TOOLCHAIN}/binrel"; \
	for t in arm-none-eabi aarch64-none-elf; do \
		f="arm-gnu-toolchain-${ARM_TOOLCHAIN}-x86_64-$t.tar.xz"; \
		wget -q --tries=5 --retry-connrefused "$base/$f"; \
		tar xf "$f"; \
		rm "$f"; \
	done

ENV PATH=/opt/arm-toolchain/arm-gnu-toolchain-${ARM_TOOLCHAIN}-x86_64-arm-none-eabi/bin:/opt/arm-toolchain/arm-gnu-toolchain-${ARM_TOOLCHAIN}-x86_64-aarch64-none-elf/bin:$PATH

# El código se monta desde fuera (otro usuario): que git no lo rechace
RUN git config --system --add safe.directory '*'

# Comprobación: que estén los dos compiladores
RUN arm-none-eabi-gcc --version | head -1 && aarch64-none-elf-gcc --version | head -1

WORKDIR /src
