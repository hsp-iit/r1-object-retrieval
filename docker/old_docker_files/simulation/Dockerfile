FROM nvidia/cuda:11.7.1-cudnn8-devel-ubuntu22.04
LABEL maintainer="Raffaele Colombo"

ARG ros_distro

# Non-interactive installation mode
ENV DEBIAN_FRONTEND=noninteractive
ENV DOCKER_UPDATED_ON=28_09_2022

# Install essentials
RUN apt-get update && apt-get install -y apt-utils software-properties-common sudo psmisc lsb-release \
    tmux nano geany vim wget curl build-essential git gitk cmake cmake-curses-gui autoconf xserver-xorg-video-dummy xserver-xorg-legacy \
    net-tools terminator libjpeg-dev ffmpeg apt-transport-https ca-certificates gnupg libace-dev ycm-cmake-modules locales \
    python3-setuptools python3-pip iproute2 python3-tornado swig lsof iftop iputils-ping gdb bash-completion btop mlocate && rm -rf /var/lib/apt/lists/*

RUN pip3 install numpy bpytop

RUN sed -i 's/allowed_users=console/allowed_users=anybody/' /etc/X11/Xwrapper.config
COPY ./common/xorg.conf /etc/X11/xorg.conf

# ROS2 Installation
RUN locale-gen en_US en_US.UTF-8
RUN update-locale LC_ALL=en_US.UTF-8 LANG=en_US.UTF-8
ENV LANG=en_US.UTF-8
RUN curl -sSL https://raw.githubusercontent.com/ros/rosdistro/master/ros.key -o /usr/share/keyrings/ros-archive-keyring.gpg
RUN echo "deb [arch=$(dpkg --print-architecture) signed-by=/usr/share/keyrings/ros-archive-keyring.gpg] http://packages.ros.org/ros2/ubuntu $(lsb_release -cs) main" | tee /etc/apt/sources.list.d/ros2.list > /dev/null
RUN apt-get update && apt-get upgrade -y && apt-get install -y ros-$ros_distro-desktop ros-$ros_distro-navigation2 ros-$ros_distro-nav2-bringup \
    ros-$ros_distro-slam-toolbox ros-$ros_distro-test-msgs ros-$ros_distro-gazebo-ros \
    python3-colcon-common-extensions python3-flake8 python3-flake8-blind-except python3-flake8-builtins \
    python3-flake8-class-newline python3-flake8-comprehensions python3-flake8-deprecated python3-flake8-docstrings \
    python3-flake8-import-order python3-flake8-quotes python3-pytest python3-pytest-cov python3-pytest-repeat \
    python3-pytest-rerunfailures python3-rosdep python3-vcstool && rosdep init && rm -rf /var/lib/apt/lists/*

# Install yarp dependencies
RUN echo "deb http://www.icub.org/ubuntu `lsb_release -cs` contrib/science" > /etc/apt/sources.list.d/icub.list
RUN apt-key adv --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 57A5ACB6110576A6
RUN apt-get update && apt-get install -y icub-common libopencv-dev && sudo rm -rf /var/lib/apt/lists/*

# Create user: user1
USER root
RUN useradd -l -u 33334 -G sudo -md /home/user1 -s /bin/bash -p user1 user1 && \
    # passwordless sudo for users in the 'sudo' group
    sed -i.bkp -e 's/%sudo\s\+ALL=(ALL\(:ALL\)\?)\s\+ALL/%sudo ALL=NOPASSWD:ALL/g' /etc/sudoers

# ROS2 Installation (part2)
USER user1
RUN rosdep fix-permissions && rosdep update

# Set ENV
USER user1
RUN mkdir /home/user1/robotology
ENV robotology_install_user user1
ENV user1_home /home/$robotology_install_user
ENV robotology_install_folder $user1_home/robotology

#gazebo models
RUN mkdir -p $user1_home/.gazebo && git clone https://github.com/osrf/gazebo_models $user1_home/.gazebo/models

# Build ycm
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/ycm.git -b master
RUN cd ycm && mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release && \
    make -j11
ENV YCM_DIR=/home/user1/robotology/ycm/build

# Build YARP
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/yarp.git -b master
RUN cd yarp && mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DYARP_COMPILE_libYARP_math=ON \
    -DYARP_COMPILE_GUIS=ON \
    -DYARP_COMPILE_DEVICE_PLUGINS=ON \
    -DENABLE_yarpcar_mjpeg=ON \
    -DENABLE_yarpcar_segmentationimage=ON \
    -DENABLE_yarpcar_portmonitor=ON \
    -DENABLE_yarpmod_portaudio=ON \
    -DENABLE_yarpmod_portaudioPlayer=ON \
    -DENABLE_yarpmod_portaudioRecorder=ON \
    -DENABLE_yarpmod_fakeAnalogSensor=ON \
    -DENABLE_yarpmod_fakeBattery=ON  \
    -DENABLE_yarpmod_fakeDepthCamera=ON \
    -DENABLE_yarpmod_fakeIMU=ON  \
    -DENABLE_yarpmod_fakeLaser=ON  \
    -DENABLE_yarpmod_fakeLocalizer=ON \
    -DENABLE_yarpmod_fakeMicrophone=ON  \
    -DENABLE_yarpmod_fakeMotionControl=ON  \
    -DENABLE_yarpmod_fakeNavigation=ON  \
    -DENABLE_yarpmod_fakeSpeaker=ON  \
    -DENABLE_yarpmod_fakebot=ON \
    -DENABLE_yarpmod_laserFromDepth=ON \
    -DENABLE_yarpmod_laserFromExternalPort=ON \
    -DENABLE_yarpmod_laserFromDepth=ON \
    -DENABLE_yarpmod_laserFromPointCloud=ON \
    -DENABLE_yarpmod_laserFromRosTopic=ON \
    -DENABLE_yarppm_soundfilter_resample=ON \
    -DENABLE_yarppm_bottle_compression_zlib=ON \
    -DENABLE_yarppm_depthimage_compression_zlib=ON \
    -DENABLE_yarppm_image_compression_ffmpeg=ON \
    -DENABLE_yarppm_depthimage_to_mono=ON \
    -DENABLE_yarppm_depthimage_to_rgb=ON && \
    make -j11 
ENV YARP_ROOT=$robotology_install_folder/yarp
ENV YARP_DIR=$robotology_install_folder/yarp/build

# YARP bindings
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN cd yarp && cd bindings &&  mkdir build && cd build && cmake .. -DCREATE_PYTHON=ON && make -j11

# Install YARP completion
RUN sudo ln -s /usr/local/share/bash-completion/completions/yarp /usr/share/bash-completion/completions

# Build icub-main
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/icub-main.git -b devel
RUN cd icub-main && mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DENABLE_icubmod_cartesiancontrollerserver=ON \
    -DENABLE_icubmod_cartesiancontrollerclient=ON \
    -DICUBMAIN_COMPILE_SIMULATORS=OFF \
    -DENABLE_icubmod_gazecontrollerclient=ON && \
    make -j11
ENV ICUB_ROOT=$robotology_install_folder/icub-main
ENV ICUB_DIR=$robotology_install_folder/icub-main/build

# Build robots-configuration devel
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/randaz81/robots-configuration.git -b R1SN003_ros2

# Build navigation
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/navigation.git -b master
RUN cd navigation && mkdir build && cd build && /bin/bash -c "source /opt/ros/$ros_distro/setup.bash; cmake .. -DCMAKE_BUILD_TYPE=Release -DNAVIGATION_USE_NWC=ON -DNAVIGATION_USE_ROS2=ON" && make -j11
ENV navigation_ROOT=$robotology_install_folder/navigation
ENV navigation_DIR=$robotology_install_folder/navigation/build

# Gazebo
RUN sudo sh -c 'echo "deb http://packages.osrfoundation.org/gazebo/ubuntu-stable `lsb_release -cs` main" > /etc/apt/sources.list.d/gazebo-stable.list'
RUN wget https://packages.osrfoundation.org/gazebo.key -O - | sudo apt-key add -
RUN sudo apt-get update && sudo apt-get install -y gazebo-common gazebo libgazebo-dev mesa-utils && sudo rm -rf /var/lib/apt/lists/*

# Gazebo Plugins
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/gazebo-yarp-plugins -b devel
RUN cd gazebo-yarp-plugins && mkdir build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DGAZEBO_YARP_PLUGINS_DISABLE_IMPLICIT_NETWORK_WRAPPERS=ON && \
    make -j4
ENV GazeboYARPPlugins_ROOT=$robotology_install_folder/gazebo-yarp-plugins
ENV GazeboYARPPlugins_DIR=$robotology_install_folder/gazebo-yarp-plugins/build

# CER
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/cer.git -b devel
RUN cd cer && mkdir build && cd build && cmake .. \
                -DCMAKE_BUILD_TYPE=Release \
                -DENABLE_cermod_cerDoubleLidar=ON \
                -DENABLE_cermod_cerOdometry=ON \
                -DENABLE_cermod_tripodMotionControl=ON && make -j11
ENV CER_ROOT=$robotology_install_folder/cer
ENV CER_DIR=$robotology_install_folder/cer/build


# YARP-DEVICES-ROS2
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/yarp-devices-ros2 -b master
RUN cd yarp-devices-ros2/ros2_interfaces_ws && /bin/bash -c "source /opt/ros/$ros_distro/setup.bash; colcon build"
RUN cd yarp-devices-ros2 && mkdir build && cd build && /bin/bash -c "source /opt/ros/$ros_distro/setup.bash; source /home/user1/robotology/yarp-devices-ros2/ros2_interfaces_ws/install/setup.bash; cmake .. -DYARP_ROS2_USE_SYSTEM_map2d_nws_ros2_msgs=ON -DYARP_ROS2_USE_SYSTEM_yarp_control_msgs=ON; make -j11"

# CER-Sim stuff
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/cer-sim -b ros2
RUN cd cer-sim/colcon_ws && /bin/bash -c "source /opt/ros/$ros_distro/setup.bash; colcon build; source /home/user1/robotology/cer-sim/colcon_ws/install/local_setup.bash"

# Build icub-contrib-common
USER $robotology_install_user
WORKDIR $robotology_install_folder
RUN git clone https://github.com/robotology/icub-contrib-common.git
RUN cd icub-contrib-common && mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=$robotology_install_folder/iCubContrib && sudo make -j11 install
ENV ICUBcontrib_DIR=$robotology_install_folder/iCubContrib/build

USER $robotology_install_user

# Set environmental variables
RUN echo "PS1='\[\e]0;\u \w\a\]\[\033[01;32m\]\u\[\033[00m\] \[\033[01;34m\]\w\[\033[00m\] \$ '" >> /home/$robotology_install_user/.bashrc
RUN echo "source /opt/ros/$ros_distro/setup.bash" >> /home/user1/.bashrc
RUN echo "source /home/user1/robotology/yarp-devices-ros2/ros2_interfaces_ws/install/local_setup.bash" >> /home/user1/.bashrc
ENV PATH=$PATH:$YARP_DIR/bin:$navigation_DIR/bin:$CER_DIR/bin:$ICUB_DIR/bin:$ICUB_DIR:$robotology_install_folder/iCubContrib/bin:/home/user1/robotology/Groot/build:/home/user1/tour-guide-robot/build/bin
ENV DISPLAY=:1
ENV YARP_DATA_DIRS=$YARP_DIR/share/yarp:$ICUB_DIR/share/iCub:$navigation_DIR/share/navigation:$user1_home/tour-guide-robot/build/share/tour-guide-robot:$CER_DIR/share/CER:$ICUBcontrib_DIR/share/ICUBcontrib:$robotology_install_folder/yarp-devices-ros2/build/share/yarp:$robotology_install_folder/yarp-devices-ros2/build/share/yarp-devices-ros2
ENV LD_LIBRARY_PATH=$robotology_install_folder/yarp/build/lib/yarp/
ENV GAZEBO_PLUGIN_PATH=$GazeboYARPPlugins_DIR/lib
ENV GAZEBO_MODEL_PATH=$robotology_install_folder/cer-sim/gazebo/
ENV YARP_COLORED_OUTPUT=1
ENV QT_X11_NO_MITSHM=1
ENV PYTHONPATH=$PYTHONPATH:/home/user1/robotology/yarp/bindings/build/lib/python3/
ENV YARP_CLOCK=/clock

# Manage yarp port
EXPOSE 10000/tcp 10000/udp

# Launch bash from $robotology_install_folder
USER $robotology_install_user
RUN sudo chown -R $robotology_install_user:$robotology_install_user /opt/ros && sudo chown -R $robotology_install_user:$robotology_install_user /home/$robotology_install_user/.ros

# Konstantinos Karapas stuff only
WORKDIR ${user1_home}
RUN git clone https://github.com/elandini84/yarp-web-teleop.git
RUN git clone https://github.com/elandini84/libfvad && cd libfvad && autoreconf -i && ./configure && sudo make install
RUN git clone https://github.com/hsp-iit/tour-guide-robot -b $ros_distro
RUN cd tour-guide-robot && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j11

# R1 NETWORK CONFIG
RUN yarp conf 172.17.0.1 10000

# Possibly temporary ROS2 network middleware change
RUN sudo apt-get update && sudo apt-get install -y cyclonedds-dev cyclonedds-tools \
    ros-$ros_distro-cyclonedds-dbgsym ros-$ros_distro-cyclonedds ros-$ros_distro-rmw-cyclonedds-cpp \
    ros-$ros_distro-rmw-cyclonedds-cpp-dbgsym libcycloneddsidl0 && \
    sudo apt-get clean && sudo rm -rf /var/lib/apt/lists/*
ENV RMW_IMPLEMENTATION=rmw_cyclonedds_cpp

# ------------------------------------------------------------------------------ #
############ mdetr things ############
# # Build multimodal
# USER user1
# WORKDIR ${user1_home}
# RUN git clone --recursive https://github.com/facebookresearch/multimodal.git multimodal
# ENV multimodal_install_folder  ${user1_home}/multimodal
# 
# # Install Mdetr requirements
# RUN pip install opencv-python
# RUN pip install timm
# RUN pip install prettytable==3.3.0
# RUN pip install pycocotools
# RUN pip install pytorch-lightning==1.6.0
# RUN pip install transformers==4.30.0
# RUN python3 -m pip install torchmultimodal-nightly
######################################

# Install YOLO requirements
RUN pip install ultralytics

# r1-object-retrieval
WORKDIR ${user1_home}
RUN git clone https://github.com/hsp-iit/r1-object-retrieval && \
    cd r1-object-retrieval && mkdir build && cd build && cmake .. && sudo make -j11
ENV PYTHONPATH=$PYTHONPATH:${user1_home}/r1-object-retrieval
ENV YARP_DATA_DIRS=$YARP_DATA_DIRS:${user1_home}/r1-object-retrieval/build/share/R1_OBJECT_RETRIEVAL
ENV PATH=${PATH}:${user1_home}/.local/bin
ENV PATH=$PATH:${user1_home}/r1-object-retrieval/build/bin
ENV R1_OBR_SOURCE_DIR=${user1_home}/r1-object-retrieval
ENV R1_OBR_BUILD_DIR=${R1_OBR_SOURCE_DIR}/build

# Install openai c++ dependencies
RUN sudo apt-get update && sudo apt-get install -y unzip curl zip
RUN git clone https://github.com/Microsoft/vcpkg.git && \
    cd vcpkg && ./bootstrap-vcpkg.sh -disableMetrics && \
    ./vcpkg integrate install && ./vcpkg install curl && \
    ./vcpkg install nlohmann-json

# Install openai c++ community library
RUN git clone https://github.com/D7EAD/liboai.git -b v4.0.0-dev && \
    cd liboai/liboai && \
    cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=/home/user1/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DCMAKE_POSITION_INDEPENDENT_CODE=true && \
    cd build && make -j11 && sudo make install

# Install yarpGPT
WORKDIR ${robotology_install_folder}
RUN git clone --branch feat/aux_module https://github.com/robotology/yarp-devices-llm.git
RUN cd ${robotology_install_folder}/yarp-devices-llm && \
    rm -rf build && \
    cmake -B build -S . \
    -DCMAKE_TOOLCHAIN_FILE=/home/user1/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DLIBOAI_INSTALL_PATH=/home/user1/liboai && cd build \ 
    && sudo make install -j11
ENV YARP_DATA_DIRS=$YARP_DATA_DIRS:${robotology_install_folder}/yarp-devices-llm/build/share/yarp
RUN echo "set -a" >> /home/user1/.bashrc
RUN echo "source ${robotology_install_folder}/yarp-devices-llm/.env" >> /home/user1/.bashrc
RUN echo "set +a" >> /home/user1/.bashrc

# Gazebo worlds
WORKDIR ${user1_home}
RUN git clone https://github.com/leonhartyao/gazebo_models_worlds_collection
ENV GAZEBO_MODEL_PATH=${GAZEBO_MODEL_PATH}:/home/user1/gazebo_models_worlds_collection/models

WORKDIR ${user1_home}/r1-object-retrieval
RUN echo "git pull" >> update_repo.sh
RUN echo "cd build && cmake .. && sudo make -j11" >> update_repo.sh

RUN echo "cd ${robotology_install_folder}/cer && git pull" >> update_repo.sh
RUN echo "cd ${robotology_install_folder}/cer/build && cmake .. && make -j11" >> update_repo.sh
RUN echo "cd ${user1_home}/tour-guide-robot && git pull" >> update_repo.sh
RUN echo "cd ${user1_home}/tour-guide-robot/build && cmake .. && make -j11" >> update_repo.sh
RUN sudo chmod 777 update_repo.sh

RUN ln -s ${user1_home}/tour-guide-robot/app/navigation2/scripts/stop.sh stop_sim.sh

ENV ROS_DOMAIN_ID=39
# ------------------------------------------------------------------------------ #

RUN sudo updatedb
CMD ["bash"]
