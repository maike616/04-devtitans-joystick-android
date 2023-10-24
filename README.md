
# DevTITANS - joystick-android

# Titanstation
O Titansation é um projeto que faz parte do programa de capacitação em desenvolvimento de Android em Sistemas Embarcados, uma iniciativa conjunta entre o Instituto de computação (IComp) a Universidade Federal do Amazonas (UFAM), a Motorola e a Flextronics. Este projeto teve como objetivo a criação de um sistema operacional Android personalizado para um controle de videogame. Ele é uma iniciativa gratuita e de código aberto. 

## Requisitos

- Sistema Operacional Linux
> Antes de iniciar o processo, é crucial garantir que o seu sistema possua idealmente 32 GB de RAM e que haja disponibilidade de 300 GB de espaço livre no disco rígido.

- Raspberry Pi 4
- Cartão SD ou Micro SD (mínimo de 8GB)
- ESP32
- Botões, sensores, jumps, protoboard

## Passos iniciais

**Instalando pacotes do sistema:**

-   [Instalando os pacotes necessários do AOSP](https://source.android.com/setup/build/initializing) .
 ```
sudo apt-get install -y git-core gnupg flex bison build-essential zip curl zlib1g-dev gcc-multilib g++-multilib libc6-dev-i386 lib32ncurses5-dev x11proto-core-dev libx11-dev lib32z1-dev libgl1-mesa-dev libxml2-utils xsltproc unzip fontconfig - sudo apt-get install adb
```
-   Instalando pacotes adicionais
```
sudo apt-get install -y swig libssl-dev flex bison device-tree-compiler mtools git gettext libncurses5 libgmp-dev libmpc-dev cpio rsync dosfstools kmod gdisk lz4 meson cmake libglib2.0-dev git-lfs
```
-   Instalando pacotes adicionais (para construir mesa3d, libcamera e outros componentes baseados em meson)
```
sudo apt-get install -y python3-pip pkg-config python3-dev ninja-build
sudo pip3 install mako jinja2 ply pyyaml pyelftools
```
-   Instale a ferramenta `repo`
```
sudo apt-get install -y python-is-python3 wget
wget -P ~/bin http://commondatastorage.googleapis.com/git-repo-downloads/repo
chmod a+x ~/bin/repo
```
 **[Baixando o GloDroid](https://glodroid.github.io/):**
 - mkdir -p aosp-raspberry
 - cd aosp-raspberry
 - repo init -u https://github.com/glodroid/glodroid_manifest -b master
 - repo sync -cq

 **Baixando o ADB (Android Open Source Project):**
 - sudo apt-get update
 - sudo apt-get install adb

**Build do GloDroid:**
- source ./build/envsetup.sh
- lunch rpi4-userdebug
- make images

 **Baixando Raspberry Pi Imager:**

> [https://www.cyberithub.com/how-to-install-raspberry-pi-imager-on-ubuntu-20-04-lts/](https://www.cyberithub.com/how-to-install-raspberry-pi-imager-on-ubuntu-20-04-lts/)

**Executando Raspberry Pi Imager**
- sudo rpi-imager
> Com Raspberry Pi Imager aberto precisamos configurar os seguintes campos:
> - Operating System → Use Custom → ~/aosp-raspberry/out/target/product/rpi4
> - Selecione o arquivo deploy-sd.img
> - Storage → Selecione o seu cartão SD
> - Clique em "Write"

> Concluídos estes passos, insira o cartão SD no Raspberry Pi e conecte o cabo USB-C na alimentação do Raspberry Pi e na porta USB do notebook
> - cd ~/aosp-raspberry/out/target/product/rpi4/
> - ./flash-sd.sh

## Preparando o Titanstation

- Faça as alterações nos arquivos de acordo com o caminho mostrado na branch linux_module
- Embarque o códico da branch esp32 no esp32
- Build novamente o código
- Faça a imagem novamente com o Raspberry Pi Imager
