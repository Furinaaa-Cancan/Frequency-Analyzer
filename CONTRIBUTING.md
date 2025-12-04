# Contributing to Frequency Analyzer

Thank you for your interest in contributing to the Frequency Analyzer project! We welcome contributions from the community.

## 📋 Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [How to Contribute](#how-to-contribute)
- [Pull Request Process](#pull-request-process)
- [Coding Standards](#coding-standards)

## 📜 Code of Conduct

Please be respectful and constructive in all interactions. We are committed to providing a welcoming and inclusive environment for everyone.

## 🚀 Getting Started

1. **Fork** the repository on GitHub
2. **Clone** your fork locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/Frequency-Analyzer.git
   cd Frequency-Analyzer
   ```
3. **Add upstream** remote:
   ```bash
   git remote add upstream https://github.com/leeyoung7017/Frequency-Analyzer.git
   ```

## 🛠️ Development Setup

### Firmware Development

1. Install [Keil MDK](https://www.keil.com/mdk5/) v5.x or later
2. Install GD32F10x Device Family Pack
3. Open `firmware/Tamlate.uvprojx`

### Web Interface Development

```bash
cd react-bode-analyzer
npm install
npm run dev
```

## 💡 How to Contribute

### Reporting Bugs

- Use the GitHub Issues page
- Include steps to reproduce
- Include expected vs actual behavior
- Include hardware/software versions

### Suggesting Features

- Open an issue with the "feature request" label
- Describe the use case and benefits
- Be open to discussion

### Code Contributions

Areas where we especially welcome contributions:

- **Firmware**: New measurement algorithms, hardware drivers
- **Web UI**: UI/UX improvements, new chart types
- **Documentation**: Tutorials, translations, examples
- **Testing**: Unit tests, integration tests

## 📤 Pull Request Process

1. Create a feature branch:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. Make your changes with clear, descriptive commits

3. Push to your fork:
   ```bash
   git push origin feature/your-feature-name
   ```

4. Open a Pull Request against `main` branch

5. Ensure your PR:
   - Has a clear description
   - References any related issues
   - Passes all checks
   - Has been tested

## 📏 Coding Standards

### C Code (Firmware)

```c
// Use descriptive function names
void ADC_DualChannel_StartSampling(void);

// Use snake_case for variables
uint32_t sample_count = 0;

// Use UPPER_CASE for constants
#define ADC_BUFFER_SIZE 512

// Add comments for complex logic
// Calculate phase using single-bin DFT
float phase = atan2f(sin_sum, cos_sum);
```

### JavaScript/React Code

```javascript
// Use PascalCase for components
function BodePlot({ data, options }) { ... }

// Use camelCase for functions and variables
const calculateGain = (input, output) => { ... }

// Use descriptive prop names
<ChartComponent
  frequencyData={bodeData}
  isLoading={isMeasuring}
/>
```

### Commit Messages

Use conventional commits:

```
feat: add phase unwrapping algorithm
fix: correct ADC sampling rate calculation
docs: update hardware connection guide
refactor: simplify DFT computation
```

## ❓ Questions?

Feel free to open an issue for any questions or reach out to the maintainers.

---

Thank you for contributing! 🎉
