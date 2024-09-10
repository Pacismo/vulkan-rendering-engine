use clap::{Parser, ValueEnum, ValueHint::FilePath};
use std::path::PathBuf;

#[derive(Debug, Clone, Copy, ValueEnum, Default)]
pub enum ShaderType {
    #[default]
    Infer,
    Vertex,
    Fragment,
}

impl ShaderType {
    pub fn to_shaderc(self) -> shaderc::ShaderKind {
        use shaderc::ShaderKind as K;
        use ShaderType::*;
        match self {
            Infer => K::InferFromSource,
            Vertex => K::Vertex,
            Fragment => K::Fragment,
        }
    }
}

#[derive(Debug, Clone, Copy, ValueEnum, Default)]
pub enum OutputMode {
    #[default]
    /// Output in binary
    Binary,
    /// Output as a comma-separated list of 8-bit integer values
    U8List,
    /// Output as a comma-separated list of 32-bit integer values
    U32List,
    /// Output as a string containing assembly code
    Assembly,
}

impl OutputMode {
    pub fn is_binary(self) -> bool {
        !matches!(self, Self::Assembly)
    }
}

#[derive(Debug, Parser)]
pub struct Arguments {
    /// The file to compile
    #[arg(value_hint=FilePath)]
    pub input: PathBuf,
    /// What kind of shader this is (leave blank to infer)
    #[arg(short, long, default_value = "infer")]
    pub kind: ShaderType,
    /// The entry point of the shader
    #[arg(short, long, default_value = "main")]
    pub entrypoint: String,
    /// The file to write the shader to
    ///
    /// Leave blank to output to stdout
    #[arg(short, long)]
    pub output: Option<PathBuf>,
    /// How to output the binary values
    #[arg(short = 'm', long = "mode", default_value = "binary")]
    pub output_mode: OutputMode,
}
