use std::{fmt::Display, io};

pub type Result<T, E = Error> = std::result::Result<T, E>;

#[derive(Debug)]
pub enum Error {
    StringError(String),
    ShaderCError(shaderc::Error),
    IoError(io::Error),
}

pub use Error::*;

impl std::error::Error for Error {
    fn source(&self) -> Option<&(dyn std::error::Error + 'static)> {
        match self {
            StringError(..) => None,
            ShaderCError(e) => Some(e),
            IoError(e) => Some(e),
        }
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            StringError(e) => e.fmt(f),
            ShaderCError(e) => e.fmt(f),
            IoError(e) => e.fmt(f),
        }
    }
}

impl From<shaderc::Error> for Error {
    fn from(value: shaderc::Error) -> Self {
        ShaderCError(value)
    }
}

impl From<String> for Error {
    fn from(value: String) -> Self {
        StringError(value)
    }
}

impl From<&str> for Error {
    fn from(value: &str) -> Self {
        StringError(value.to_owned())
    }
}

impl From<io::Error> for Error {
    fn from(value: io::Error) -> Self {
        IoError(value)
    }
}
