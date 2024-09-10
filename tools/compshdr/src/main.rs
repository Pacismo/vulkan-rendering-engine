mod cli;
mod error;
use std::{
    fs::{read_to_string, File},
    io::{self, stdout, Write},
};

use cli::Arguments;
use error::*;

use clap::Parser;
use shaderc::{CompilationArtifact, Compiler};

fn write_as_binary<W>(mut out: W, data: CompilationArtifact) -> io::Result<()>
where
    W: Write,
{
    out.write_all(data.as_binary_u8())
}

fn write_as_u8<W>(mut out: W, data: CompilationArtifact) -> io::Result<()>
where
    W: Write,
{
    for byte in data.as_binary_u8() {
        write!(out, "0x{byte:02X}")?;
    }

    Ok(())
}

fn write_as_u32<W>(mut out: W, data: CompilationArtifact) -> io::Result<()>
where
    W: Write,
{
    let data = data.as_binary();
    let count = data.len() * 4;

    for (i, word) in data.iter().enumerate() {
        if i == count - 1 {
            write!(out, "0x{word:08X}")?;
        } else if i % 4 == 3 {
            writeln!(out, "0x{word:08X},")?;
        } else {
            write!(out, "0x{word:08X}, ")?;
        }
    }

    Ok(())
}

fn write_as_asm<W>(mut out: W, data: CompilationArtifact) -> io::Result<()>
where
    W: Write,
{
    let data = data.as_binary_u8();
    let count = data.len();
    for (i, byte) in data.iter().enumerate() {
        if i == count - 1 {
            write!(out, "0x{byte:02X}")?;
        } else if i % 8 == 7 {
            writeln!(out, "0x{byte:02X},")?;
        } else {
            write!(out, "0x{byte:02X}, ")?;
        }
    }

    Ok(())
}

fn main() -> Result<()> {
    let Arguments {
        input,
        kind,
        entrypoint,
        output,
        output_mode,
    } = Arguments::parse();

    let fname = input
        .file_name()
        .ok_or("Failed to get file name")?
        .to_string_lossy()
        .to_string();
    let fdata = read_to_string(input)?;

    let compiler = Compiler::new().ok_or("Failed to get access to the compiler")?;

    let artifact = if output_mode.is_binary() {
        compiler.compile_into_spirv(&fdata, kind.to_shaderc(), &fname, &entrypoint, None)?
    } else {
        compiler.compile_into_spirv_assembly(
            &fdata,
            kind.to_shaderc(),
            &fname,
            &entrypoint,
            None,
        )?
    };

    if let Some(path) = output {
        let file = File::create(path)?;
        match output_mode {
            cli::OutputMode::Binary => write_as_binary(file, artifact)?,
            cli::OutputMode::U8List => write_as_u8(file, artifact)?,
            cli::OutputMode::U32List => write_as_u32(file, artifact)?,
            cli::OutputMode::Assembly => write_as_asm(file, artifact)?,
        }
    } else {
        match output_mode {
            cli::OutputMode::Binary => write_as_binary(stdout(), artifact)?,
            cli::OutputMode::U8List => write_as_u8(stdout(), artifact)?,
            cli::OutputMode::U32List => write_as_u32(stdout(), artifact)?,
            cli::OutputMode::Assembly => write_as_asm(stdout(), artifact)?,
        }
    }

    Ok(())
}
