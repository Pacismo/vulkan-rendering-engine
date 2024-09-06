mod error;
use error::{Error::*, Result};

use clap::{Parser, ValueHint::FilePath};
use std::{
    fs::read,
    io::{stdout, Write},
    path::PathBuf,
};

#[derive(Parser)]
struct Args {
    #[clap(value_hint = FilePath)]
    file: PathBuf,
    #[clap(short = 'l', long)]
    as_u32: bool,
}

fn main() -> Result<()> {
    let Args { file, as_u32 } = Args::parse();

    if !file.exists() {
        return Err(FileNotFound(file));
    }

    let data = read(&file)?;
    let bytes = data.len();

    if as_u32 {
        if bytes % 4 != 0 {
            return Err(NotMultipleOfFour);
        }

        for (i, word) in data.chunks_exact(4).enumerate() {
            let word = u32::from_le_bytes([word[0], word[1], word[2], word[3]]);
            if i == (bytes / 4) - 1 {
                print!("0x{:08X}", word);
            } else if i % 4 == 3 {
                println!("0x{:08X},", word);
            } else {
                print!("0x{:08X}, ", word);
            }
        }
    } else {
        for (i, byte) in data.into_iter().enumerate() {
            if i == bytes - 1 {
                print!("0x{:02X}", byte);
            } else if i % 8 == 7 {
                println!("0x{:02X},", byte);
            } else {
                print!("0x{:02X}, ", byte);
            }
        }
    }

    stdout().flush()?;

    Ok(())
}
