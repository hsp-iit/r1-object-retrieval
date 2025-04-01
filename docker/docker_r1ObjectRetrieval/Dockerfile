ARG base_img
FROM ${base_img}
# FROM elandini84/r1images:tourSim2_cuda.11.7.1-cudnn8_iron_devel
LABEL maintainer="Ettore Landini"

ARG user_name=user1



# ------------------------------------------------------------------------------ #

############ mdetr things ############
# # Build multimodal
# USER ${user_name}
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
RUN pip install --break-system-packages ultralytics

# r1-object-retrieval
WORKDIR ${robotology_install_folder}
USER ${user_name}
RUN git clone https://github.com/hsp-iit/r1-object-retrieval
RUN cd r1-object-retrieval && mkdir build && cd build && cmake .. && make -j11
ENV PYTHONPATH=$PYTHONPATH:${robotology_install_folder}/r1-object-retrieval
ENV YARP_DATA_DIRS=$YARP_DATA_DIRS:${robotology_install_folder}/r1-object-retrieval/build/share/R1_OBJECT_RETRIEVAL
ENV PATH=${PATH}:/home/${robotology_install_user}/.local/bin
ENV PATH=$PATH:${robotology_install_folder}/r1-object-retrieval/build/bin
ENV R1_OBR_SOURCE_DIR=${robotology_install_folder}/r1-object-retrieval
ENV R1_OBR_BUILD_DIR=${R1_OBR_SOURCE_DIR}/build

WORKDIR ${robotology_install_folder}/r1-object-retrieval
RUN echo "git pull" >> update_repo.sh
RUN echo "cd build && cmake .. && make -j11" >> update_repo.sh
RUN echo "cd ${robotology_install_folder}/cer && git pull" >> update_repo.sh
RUN echo "cd ${robotology_install_folder}/cer/build && cmake .. && make -j11" >> update_repo.sh
RUN echo "cd ${robotology_install_folder}/tour-guide-robot && git pull" >> update_repo.sh
RUN echo "cd ${robotology_install_folder}/tour-guide-robot/build && cmake .. && make -j11" >> update_repo.sh
RUN sudo chmod 777 update_repo.sh


# ------------------------------------------------------------------------------ #

CMD ["bash"]
