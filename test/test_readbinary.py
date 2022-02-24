import ReadBinary

pp = ReadBinary.ReadBlock("3V41_LED_55V_SiPM.bin")

pp["sipm_traces"]

print(pp)