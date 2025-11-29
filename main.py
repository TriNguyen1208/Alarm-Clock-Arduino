import pickle
import argparse

model = pickle.load(open('model.pkl', mode='rb'))
def main():
    parser = argparse.ArgumentParser(description="Process sensor data: temperature, humidity, air quality (ppm).")
    parser.add_argument("--temp", type=float, required=True, help="Temperature in Celsius")
    parser.add_argument("--humid", type=float, required=True, help="Humidity in percentage")
    parser.add_argument("--ppm", type=float, required=True, help="Air quality (PPM)")
    args = parser.parse_args()

    temperature = args.temp
    humidity = args.humid
    ppm = args.ppm

    pred = model.predict([[temperature, humidity, ppm]])
    if pred == 0:
        print("No pollution")
    else:
        print("Pollution")


if __name__ == '__main__':
    main()