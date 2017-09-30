import random
import os
import sys


class Animal(object):

    # 2 underscores make them private
    _name = ""
    __height = 0
    __weight = 0
    __sound = 0
    pub = 7

    # constructor in python
    def __init__(self, name, height, weight, sound):
        self._name = name
        self.__height = height
        self.__weight = weight
        self.__sound = sound

    # Setters and Getters
    def set_name(self, name):
        self._name = name
    def get_name(self):
        return self._name
    def set_height(self,height):
        self.__height = height
    def get_height(self):
        return self.__height
    def set_weight(self,weight):
        self.__weight = weight
    def get_weight(self):
        return self.__weight
    def set_sound(self,sound):
        self.__sound = sound
    def get_sound(self):
        return self.__sound
    def get_type(self):
        print("Animal")

    def toString(self):
        return "{} is {} cm tall and {} kg and speaks {}".format(self._name,
                                                                 self.__height,
                                                                 self.__weight,
                                                                 self.__sound);



cat = Animal("Mercy",33,10,"Meow")

#print(cat.toString())

# Inheritance
class Dog(Animal):
    __owner = ""

    def __init__(self, name, height, weight, sound, owner):
        self.__owner = owner
        Animal.__init__(self,name,height,weight,sound)

    def set_owner(self,owner):
        self.__owner = owner
    def get_owner(self):
        return self.__owner

    ## cannot use private variables. Getters and setters are used to access them
    def toString(self):
        return "{} is {} cm tall and {} kg and speaks {} and owner is {}".format(
               self._name,self.get_height(),self.get_weight(),self.get_sound(),self.__owner);

    def get_type(self):
        print("Dog")

    def multiple_sounds(self,how_many=None):
        if how_many is None:
            print(self.get_sound())
            print(self.pub)
        else:
            print(self.get_sound() * how_many)

louie = Dog("Louie",51,12,"Ruff","Shem")

print(louie.toString())
print(louie.get_sound())

louie.multiple_sounds()
