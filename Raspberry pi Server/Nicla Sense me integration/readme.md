 ihave previousely built a nicla hat for my raspberry pi. to get it to work, you first need to enable serial interface in the raspberypi ocnfiguration.

 then:
 python3 -m venv ~/nicla_env
activate the env:
source ~/nicla_env/bin/activate

pip install pyserial paho-mqtt
then run the code in the, you need to adjust fot the hoemassitance ever ip, before that make sure you added the mqtt ad on: In Home Assistant, go to Settings > Add-ons > Mosquitto broker.
nicla_ha_publisher.py