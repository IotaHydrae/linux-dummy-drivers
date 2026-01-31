use spidev::{SpiModeFlags, Spidev, SpidevOptions};
use std::io::Write;

fn main() {
    println!("Hello, world!");

    let mut spi = Spidev::open("/dev/spidev0.0").unwrap();

    let options = SpidevOptions::new()
        .bits_per_word(8)
        .max_speed_hz(1_000_000)
        .mode(SpiModeFlags::SPI_MODE_0)
        .build();

    spi.configure(&options).unwrap();

    let payload: Vec<u8> = vec![0x9F, 0x00, 0x00, 0x00];

    println!("Payload: {:?}", payload);

    let usize = spi.write(&payload).unwrap();

    println!("Write {}", usize);
}
