Çalışırken izlemek için: https://youtu.be/i7sEzjXTG-Q

To watch it in action: https://youtu.be/i7sEzjXTG-Q

English is just below ;)

## SAAT

*SAAT* dediğimiz şey nedir ki? Zamanı falan gösteren bir şey. Bu projede bir saatin sahip olması gereken her şey ve daha fazlası var. Kullandığı sıcaklık sensörü sayesinde bulunduğun ortamı ne kadar ısıttığını bulabilir, kronometre sayesinde ise ocaktaki yumurtayı en sevdiğin şekilde pişirebilirsin.



#### NASIL

Soracaksınız böyle harika bir cihaza nasıl kavuşurum, nasıl kullanırım. Endişelenmeyin, ufak, dikdörtgen ve plastikten yapılma kredi kartına ve zaman denilen bir şeye ihtiyacınız var.

##### LİSTE

Neye ihtiyacınız var?

- Arduino pro mini / nano => Sırf küçük diye daha büyük modelleri kullanabilirsiniz.
- DS3231 RTC chip => Yılda sadece 1 dk hata yapan bir gerçek zamanlı saat modülü
- MAX7219 Led sürücü => Bunun sayesinde sadece Arduinonun 3 pini ile 8 adete kadar 8 segment ekran sürebilirsiniz.
- DS18B20 Sıcaklık sensörü => Eğer  saatiniz oda sıcaklığı da göstersin diyorsanız kullanabilirsiniz.
- LDR Işığa hassas direnç => Eğer saatiniz odanın ışık seviyesine göre parlaklık ayarlasın diyorsanız kullanabilirsiniz.
- Button x 4adet => Modlar ve ayar için kullanacağız.
- Micro USB breakout => Güç sağlamak için.
- 7 segment ekran x 4 adet => Ekran olarak ihtiyacımız var. Ben 38mm olanlardan aldım. Alırken ortak katot veya common cathode olarak alın.
- Delikli pertinaks, perfboard, protoboard => Montaj için kullanacağız.
- 1x 1uF elektrolitik kapasitör, RTC çip için 
- 1x 100uF elektrolitik kapasitör, MAX7219 için
- 1x 100nF seramic kapasitör, MAX7219 için
- 5 x 10k ohm direnç
- 1 x 100 to 470 ohm direnç, Ekran noktaları için.

#### Eksikler

Şu anda kod benim elimde bulunan cihazda çalışacak şekilde. Ve benim elimdeki cihazda yukarıda belirttiğim bütün araçlar var. O yüzden sizde eksik varsa kodu düzeltmeden çalışmayacaktır. Ama sonunda kodun başında değişkenleri belirterek sizin donanımınızla düzgün çalışmasını sağlamak istiyorum.

- [x] Sıcaklık birimini değiştirmek
- [ ] Işık sensörünü devre dışı bırak
- [ ] Sıcaklık sensörünü devre dışı bırak
- [ ] Saat modülünü değiştirebilmek. (Sadece Adafruit RTCLib içindekiler için.)

###### Gerekli Kütüphaneler

LedControl, RTCLib - Adafruit, Bounce2, OneWire, DallasTemperature, Wire

Gerekli şematiğe [bu adresten](https://easyeda.com/abdullahcanakci/clock_) veya repo içinde Türkçe_Şematik.pdf dosyasından ulaşabilirsiniz.





## CLOCK

What is *CLOCK?* It is a <u>something</u> that shows time and such. This project has everything that a clock needs and more. It has a temperature sensor to check how hot you are inside. It has a stopwatch to track your eggs cooking and nothing more.

#### HOW

You have questions, "How can I have this fabulous device?", "How can I use this?". Fret not, I'm here to rescue you. You only need a credit card thingy and something called time.

##### BOM

What do I need?

- Arduino pro mini / nano => Just because it is small. You can use larger ones too.
- DS3231 RTC chip => This RTC chip is one of the better ones. It has accuracy of 1 minute per year.
- MAX7219 Led driver => With this driver chip, you can drive up to 8, 8 segment displays just using 3 pins of an Arduino.
- DS18B20 Temperature Sensor => This sensor is one of the better ones. If you want your clock to show temperature.
- LDR Light sensitive resistor => If you want your clock to auto set brigthness of displays you need this.
- Button x 4 piece => To switch between modes and setting up clock.
- Micro USB breakout => To provide power
- 7 segment displays x 4 => We certainly need this. I bought 38 mm one. Displays need to be common cathode
- Perfboard, protoboard => Needed for assembly.
- 1x 1uF electrolytic capacitor for RTC chip
- 1x 100uF electrolytic capacitor for MAX7219
- 1x 100nF seramic capacitor for MAX7219
- 5 x 10k ohm resistors.
- 1 x 100 to 470 ohm resistor for segment dot. 

### Shortcomings

At the release of this code base I assumed every component it included and wrote my code accordingly. But this creates a problem if you don't have everything as I have. You can edit this according to your needs, but I want it in a way you set up some parameters on top of the file and everything runs nicely with your hardware. 

- [x] Changing temperature unit.
- [ ] Disable LDR
- [ ] Disable temperature sensor
- [ ] Change RTC chip type. (Only from chips within RTCLib from Adafruit.)

###### Needed Libraries

LedControl, RTCLib - Adafruit, Bounce2, OneWire, DallasTemperature, Wire



You can access to the schematic from [this link](https://easyeda.com/abdullahcanakci/clock_) or English_Schematic.pdf file from the repository.



```
MIT License

Copyright (c) 2019 Abdullah Çanakçı

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```