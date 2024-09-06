use std::{error::Error as StdError, fmt::Display, io::Error as IoError, path::PathBuf};

pub type Result<T, E = Error> = std::result::Result<T, E>;

#[derive(Debug)]
pub enum Error {
    FileNotFound(PathBuf),
    IoError(IoError),
    NotMultipleOfFour,
}

impl StdError for Error {
    fn source(&self) -> Option<&(dyn StdError + 'static)> {
        use Error::*;
        match self {
            FileNotFound(..) => None,
            IoError(e) => Some(e),
            NotMultipleOfFour => None,
        }
    }
}

impl Display for Error {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        use Error::*;
        match self {
            FileNotFound(p) => write!(f, "File \"{}\" not found", p.display()),
            IoError(e) => e.fmt(f),
            NotMultipleOfFour => write!(f, "File contents was not a multiple of four"),
        }
    }
}

impl From<IoError> for Error {
    fn from(value: IoError) -> Self {
        Self::IoError(value)
    }
}
