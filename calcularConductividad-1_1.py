#!/usr/bin/env python
# -*- coding: cp1252 -*-
"""
    Cálculo aproximado de la conductividad de un suelo a partir de la
    medida según el esquema MedidorConductividadSuelo.sch
    Se usa para obtener el Kr y para comprobar los resultados del
    programa en Arduino
    
    v-1_1-20130226
        
    con mi sonda tengo los siguientes valores en uS/cm :
    
    agua con sal         98
    agua del grifo       45
    a los 5min. de regar 54
    maceta seca          15

    Para el conjunto agua+terreno los valores son distintos
    dependiendo de la estructura y naturaleza del terreno y de los
    iones incorporados al agua procedentes por ejemplo de los abonos.
        
    Es importante hacer ensayos con el tipo de substrato que se está usando,
    midiendo en seco, con riego abundante y con abono incorporado para
    poder establecer las referencias.
    En las medidas de conductividad hay mucha dispersión y hay que tener en
    cuenta la temperatura, ya que la conductividad aumenta del 2% al 3% por ºC

    El cálculo de la conductancia es aproximado, ya que he supuesto una
    distribución uniforme del campo eléctrico entre los electrodos sin tener
    en cuenta ni la irregularidad del contenido ni el efecto de los extremos.
    Lo que se pretende es un sistema simple para establecer medidas
    comparativas útiles en el control de riegos y abonados.

    Para otras dimensiones de la sonda u otras características del terrenno,
    se puede ajustar la resistencia del circuito a fin de mantener las
    lecturas del AD en el rango más adecuado.     
"""

#---------------------------------------------------------------------------------------------------

import math

# la sonda que he usado está formada por dos varillas paralelas hundidas en
# el suelo, de longitud L y diámetro D separadas una distancia W (datos en cm)
L = 7.5
D = 0.5
W = 1.1

# la constante Kr del electrodo es:
Kr=math.log((W*2.0/D)-1)/(L*math.pi)
# en el display la multiplicamos por 1e6, que es el valor que se usa en el
# programa de Arduino para obtener uS/cm
# la conductividad g a partir de la R medida es
# g = Kr/R

# constantes eléctricas del circuito
R1 = 2200.0     # en serie con la sonda
AD_max = 1023   # valor máximo de la lectura del AD

def calcularConductividad(lecturaAD,K):
    """calcularConductividad

        calcular la conductividad en uS/cm a partir de la lectura AD
        y el coeficiente de la sonda
        
    """
    conductanciaSonda=K/calcularResistencia(lecturaAD)
    return (conductanciaSonda*1e6)

def calcularResistividad(lecturaAD,K):
    """calcularResistividad

        calcular la resistividad en ohm/cm a partir de la lectura AD
        y el coeficiente de la sonda
        
    """
    resistividadSonda=calcularResistencia(lecturaAD)/K
    return resistividadSonda
 
def calcularResistencia(lecturaAD):
    """calcularResistencia

        calcular la resistencia equivalente en ohm de la lectura del AD
        
    """
    return R1*lecturaAD/(AD_max-lecturaAD)

# valor del AD leído en el Arduino correspondiente al sensor
AD=394

print 'AD            = %i' % AD
print 'R1            = %i ohm' % R1
print 'Kr*1e6        = %0.3f 1/cm' % (Kr*1e6)
print 'resistencia   = %0.1f ohm' % calcularResistencia(AD)
print 'resistividad  = %0.1f ohm*cm' % calcularResistividad(AD,Kr)
print 'conductividad = %0.1f uS/cm' % calcularConductividad(AD,Kr)
