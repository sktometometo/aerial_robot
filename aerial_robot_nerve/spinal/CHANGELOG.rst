^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package spinal
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.2.1 (2021-02-17)
------------------
* Fix bugs in spinal regarding pwm test (`#440 <https://github.com/JSKAerialRobot/aerial_robot/issues/440>`_)
* Replace the old repositoty name in CHANGELOG.rst (`#434 <https://github.com/JSKAerialRobot/aerial_robot/issues/434>`_)
* Increase the initial IMU calibration duration since of the increase of the spinal initilization time (`#411 <https://github.com/JSKAerialRobot/aerial_robot/issues/411>`_)
* Fixed bug of rosserial data buffer overflow add delay for Dynamixel XL430  (`#409 <https://github.com/JSKAerialRobot/aerial_robot/issues/409>`_)


1.2.0 (2020-05-31)
------------------
* Refactor aeiral_robot_base (`#406 <https://github.com/JSKAerialRobot/aerial_robot/issues/406>`_)
* Implement several extra control methods for dragon (`#401 <https://github.com/JSKAerialRobot/aerial_robot/issues/401>`_)
* Add robot namespace for whole system (`#399 <https://github.com/JSKAerialRobot/aerial_robot/issues/399>`_)
* Refactor Gazebo system (`#391 <https://github.com/JSKAerialRobot/aerial_robot/issues/391>`_)

1.1.1 (2020-04-19)
------------------
* Make the magnetic declination configurable from ros and rqt_qui, and augument the state publish from GPS. (`#395 <https://github.com/JSKAerialRobot/aerial_robot/issues/395>`_)
* Abolish the tranformation calculation from ellipsoid to sphere in magnetometer calibration (`#372 <https://github.com/JSKAerialRobot/aerial_robot/issues/372>`_)
* Change GPS location variable type (`#374 <https://github.com/JSKAerialRobot/aerial_robot/issues/374>`_)
* Increase the waiting time for the external magnetometer (Ublox) to start (`#368 <https://github.com/JSKAerialRobot/aerial_robot/issues/368>`_)
* Publish flight_state to provide the current state (`#365 <https://github.com/JSKAerialRobot/aerial_robot/issues/365>`_)

1.1.0 (2020-01-06)
------------------
* Improve pwm saturation measures (`#362 <https://github.com/JSKAerialRobot/aerial_robot/issues/362>`_)
* Make the ADC scale in battery status configurable (`#358 <https://github.com/JSKAerialRobot/aerial_robot/issues/358>`_)
* Refactoring of IMU calibration system (`#357 <https://github.com/JSKAerialRobot/aerial_robot/issues/357>`_)
* Fix bug: the wrong magnetometer value publishing in dragon model (`#355 <https://github.com/JSKAerialRobot/aerial_robot/issues/355>`_)
* Add hydrus_xi (full_acutated model) model (`#340 <https://github.com/JSKAerialRobot/aerial_robot/issues/340>`_)
* Neuron Improvement in servo connection (`#329 <https://github.com/JSKAerialRobot/aerial_robot/issues/329>`_)
* Correct the compile configuration for spinal and neuron f1 (`#311 <https://github.com/JSKAerialRobot/aerial_robot/issues/311>`_)
* Implement dynamixel TTL and RS485 mixed mode (`#320 <https://github.com/JSKAerialRobot/aerial_robot/issues/320>`_)
* Increase the  max registration size for ros publisher/subscirber in spinal (`#321 <https://github.com/JSKAerialRobot/aerial_robot/issues/321>`_)
* Fix bug about servo and add GUI in Spinal (`#302 <https://github.com/JSKAerialRobot/aerial_robot/issues/302>`_, `#308 <https://github.com/JSKAerialRobot/aerial_robot/issues/308>`_)

1.0.4 (2019-05-23)
------------------
* Add the torque enable/disable flag for extra servo directly connected with spinal (`#252 <https://github.com/JSKAerialRobot/aerial_robot/issues/252>`_)
* Add attitude_flag when checking the timeout of the flight command (`#299 <https://github.com/JSKAerialRobot/aerial_robot/issues/299>`_)
* Add actuators (e.g. joint, gimbal) disable process in force landing phase (`#290 <https://github.com/JSKAerialRobot/aerial_robot/issues/290>`_)

1.0.3 (2019-01-08)
------------------
* Fix the wrong error id  from rosserial in spinal (`#264 <https://github.com/JSKAerialRobot/aerial_robot/issues/264>`_)
* Add TIM5 for reserve servo control timer in spinal (`#237 <https://github.com/JSKAerialRobot/aerial_robot/issues/237>`_)
* Update the voltage process in spinal by removing the voltage divider (`#245 <https://github.com/JSKAerialRobot/aerial_robot/issues/245>`_)
* Add the logging via rosseial for the flight control debug (`#239 <https://github.com/JSKAerialRobot/aerial_robot/issues/239>`_)
* Fix the inactive problem of GPS module in Spinal (`#241 <https://github.com/JSKAerialRobot/aerial_robot/issues/241>`_)

1.0.2 (2018-11-24)
------------------

1.0.1 (2018-11-05)
------------------
* Add stm32f4 version neuron project (#213)

1.0.0 (2018-09-26)
------------------
* first formal release
* Contributors: Moju Zhao, Tomoki Anzai, Fan Shi
